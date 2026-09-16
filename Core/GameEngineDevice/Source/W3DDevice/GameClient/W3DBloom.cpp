/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// W3DBloom.cpp ////////////////////////////////////////////////////////////////////////////////
// Bloom post effect for additive particles, built from DX8 render targets and fixed function quads
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "Lib/BaseType.h"
#include "WWLib/always.h"
#include "W3DDevice/GameClient/W3DBloom.h"
#include "Common/GlobalData.h"
#include "Common/GameMemory.h"
#include "WW3D2/texture.h"
#include "WW3D2/surfaceclass.h"
#include "WW3D2/dx8wrapper.h"
#include "WW3D2/dx8vertexbuffer.h"
#include "WW3D2/dx8indexbuffer.h"
#include "WW3D2/dx8renderer.h"
#include "WW3D2/formconv.h"
#include "WW3D2/rinfo.h"
#include "WW3D2/camera.h"
#include "WW3D2/vertmaterial.h"
#include "WW3D2/shader.h"

W3DBloom *TheW3DBloom = nullptr;

// The blur runs at this fraction of the screen on each axis. Half keeps the halo smooth; a quarter
// shows its own texels as blocks once the glow is bright.
enum { BLOOM_DOWNSCALE = 2, BLOOM_BLUR_PASSES = 4 };

// A separable gaussian, run as a horizontal pass then a vertical one. Each tap sits between two
// texels so bilinear filtering folds two samples into one, which keeps a wide radius cheap. Four
// diagonal taps of equal weight would be a box blur instead, and a box stays square however many
// times it runs, which is what made the halo look blocky.
static const Real BLOOM_TAP_OFFSET[3] = { 0.0f, 1.3846154f, 3.2307692f };
static const Real BLOOM_TAP_WEIGHT[3] = { 0.2270270f, 0.3162162f, 0.0702703f };

// screen space quads; the copy variant replaces the target instead of adding to it
static ShaderClass makeQuadShader(Bool additive)
{
	ShaderClass shader = ShaderClass::_PresetAdditiveShader;
	shader.Set_Depth_Compare(ShaderClass::PASS_ALWAYS);
	shader.Set_Depth_Mask(ShaderClass::DEPTH_WRITE_DISABLE);
	shader.Set_Cull_Mode(ShaderClass::CULL_MODE_DISABLE);
	shader.Set_Primary_Gradient(ShaderClass::GRADIENT_MODULATE);
	if (!additive)
	{
		shader.Set_Dst_Blend_Func(ShaderClass::DSTBLEND_ZERO);
	}
	return shader;
}

static TextureClass *createTarget(Int width, Int height, WW3DFormat format)
{
	TextureClass *target = MSGNEW("TextureClass") TextureClass(width, height, format, MIP_LEVELS_1, TextureClass::POOL_DEFAULT, true);
	if (target->Peek_D3D_Texture() == nullptr)
	{
		REF_PTR_RELEASE(target);
		return nullptr;
	}

	// the targets are not powers of two, which most hardware only samples correctly when clamped
	TextureFilterClass &filter = target->Get_Filter();
	filter.Set_U_Addr_Mode(TextureFilterClass::TEXTURE_ADDRESS_CLAMP);
	filter.Set_V_Addr_Mode(TextureFilterClass::TEXTURE_ADDRESS_CLAMP);
	filter.Set_Min_Filter(TextureFilterClass::FILTER_TYPE_FAST);
	filter.Set_Mag_Filter(TextureFilterClass::FILTER_TYPE_FAST);
	filter.Set_Mip_Mapping(TextureFilterClass::FILTER_TYPE_NONE);
	return target;
}

W3DBloom::W3DBloom()
{
	m_fullTarget = nullptr;
	m_blurTarget[0] = nullptr;
	m_blurTarget[1] = nullptr;
	m_defaultTarget = nullptr;
	m_defaultDepth = nullptr;
	m_width = 0;
	m_height = 0;
	m_disabled = false;
}

W3DBloom::~W3DBloom()
{
	ReleaseResources();
}

void W3DBloom::ReleaseResources()
{
	releaseTargets();
	releaseDefaults();
	m_disabled = false;
}

Bool W3DBloom::acquireTargets(Int width, Int height, WW3DFormat format)
{
	m_fullTarget = createTarget(width, height, format);
	m_blurTarget[0] = createTarget(width / BLOOM_DOWNSCALE, height / BLOOM_DOWNSCALE, format);
	m_blurTarget[1] = createTarget(width / BLOOM_DOWNSCALE, height / BLOOM_DOWNSCALE, format);
	if (m_fullTarget == nullptr || m_blurTarget[0] == nullptr || m_blurTarget[1] == nullptr)
	{
		releaseTargets();
		return false;
	}

	m_width = width;
	m_height = height;
	return true;
}

void W3DBloom::releaseTargets()
{
	REF_PTR_RELEASE(m_fullTarget);
	REF_PTR_RELEASE(m_blurTarget[0]);
	REF_PTR_RELEASE(m_blurTarget[1]);
	m_width = 0;
	m_height = 0;
}

void W3DBloom::releaseDefaults()
{
	if (m_defaultTarget)
	{
		m_defaultTarget->Release();
		m_defaultTarget = nullptr;
	}
	if (m_defaultDepth)
	{
		m_defaultDepth->Release();
		m_defaultDepth = nullptr;
	}
}

// every target borrows the screen's depth buffer, which DX8 allows because none is larger than it
Bool W3DBloom::setTarget(TextureClass *target)
{
	SurfaceClass *surface = target->Get_Surface_Level();
	if (surface == nullptr)
	{
		return false;
	}

	HRESULT hr = DX8Wrapper::_Get_D3D_Device8()->SetRenderTarget(surface->Peek_D3D_Surface(), m_defaultDepth);
	REF_PTR_RELEASE(surface);
	return SUCCEEDED(hr);
}

Bool W3DBloom::begin(RenderInfoClass &rinfo)
{
	const Bool wanted = !m_disabled && TheGlobalData->m_useBloom && TheGlobalData->m_bloomStrength > 0.0f;

	// the meshes of this frame are already drawn, so this decides the capture for the next one
	DX8MeshRendererClass::Enable_Bloom_Capture(wanted);
	if (!wanted)
	{
		TheDX8MeshRenderer.Clear_Bloom_Lists();
		return false;
	}

	IDirect3DDevice8 *device = DX8Wrapper::_Get_D3D_Device8();
	if (FAILED(device->GetRenderTarget(&m_defaultTarget)) || FAILED(device->GetDepthStencilSurface(&m_defaultDepth)))
	{
		releaseDefaults();
		return false;
	}

	D3DSURFACE_DESC targetDesc;
	D3DSURFACE_DESC depthDesc;
	m_defaultTarget->GetDesc(&targetDesc);
	m_defaultDepth->GetDesc(&depthDesc);

	// DX8 cannot redirect a multisampled scene into a texture
	if (targetDesc.MultiSampleType != D3DMULTISAMPLE_NONE || depthDesc.MultiSampleType != D3DMULTISAMPLE_NONE)
	{
		releaseDefaults();
		return false;
	}

	if (m_fullTarget == nullptr || (Int)targetDesc.Width != m_width || (Int)targetDesc.Height != m_height)
	{
		releaseTargets();
		if (!acquireTargets(targetDesc.Width, targetDesc.Height, D3DFormat_To_WW3DFormat(targetDesc.Format)))
		{
			releaseDefaults();
			return false;
		}
	}

	if (!setTarget(m_fullTarget))
	{
		releaseTargets();
		releaseDefaults();
		m_disabled = true;
		return false;
	}

	// the new target starts with a full size viewport, so this clears all of it
	DX8Wrapper::Clear(true, false, Vector3(0.0f, 0.0f, 0.0f));
	rinfo.Camera.Apply();
	return true;
}

void W3DBloom::end(RenderInfoClass &rinfo)
{
	static const ShaderClass addShader = makeQuadShader(TRUE);
	static const ShaderClass copyShader = makeQuadShader(FALSE);

	// additive meshes join the particles in the bloom target
	TheDX8MeshRenderer.Flush_Bloom();

	Matrix4x4 identity(true);
	DX8Wrapper::Set_Transform(D3DTS_WORLD, identity);
	DX8Wrapper::Set_Transform(D3DTS_VIEW, identity);
	DX8Wrapper::Set_Transform(D3DTS_PROJECTION, identity);

	VertexMaterialClass *material = VertexMaterialClass::Get_Preset(VertexMaterialClass::PRELIT_DIFFUSE);
	DX8Wrapper::Set_Material(material);
	REF_PTR_RELEASE(material);

	// shrink the scene into the first blur target, averaging a 2x2 block per texel
	Bool drawn = blurPass(m_fullTarget, m_blurTarget[0], 0.5f / m_width, 0.5f / m_height, TRUE);

	// alternating the axis makes the kernel separable, and each pair widens the halo
	const Real blurWidth = (Real)(m_width / BLOOM_DOWNSCALE);
	const Real blurHeight = (Real)(m_height / BLOOM_DOWNSCALE);
	for (Int pass = 0; drawn && pass < BLOOM_BLUR_PASSES; ++pass)
	{
		const Real spread = 1.0f + (Real)(pass / 2);
		const Bool horizontal = (pass & 1) == 0;
		drawn = blurPass(m_blurTarget[pass & 1], m_blurTarget[(pass + 1) & 1],
			horizontal ? spread / blurWidth : 0.0f,
			horizontal ? 0.0f : spread / blurHeight,
			FALSE);
	}

	DX8Wrapper::_Get_D3D_Device8()->SetRenderTarget(m_defaultTarget, m_defaultDepth);
	rinfo.Camera.Apply();

	if (drawn)
	{
		DX8Wrapper::Set_Transform(D3DTS_VIEW, identity);
		DX8Wrapper::Set_Transform(D3DTS_PROJECTION, identity);

		// the blur covers the whole screen, the quad only the camera's part of it
		Vector2 viewMin;
		Vector2 viewMax;
		rinfo.Camera.Get_Viewport(viewMin, viewMax);
		TextureClass *glow = m_blurTarget[BLOOM_BLUR_PASSES & 1];
		if (TheGlobalData->m_bloomDebug)
		{
			// the glow alone on black, so it is obvious whether anything reached the target
			drawQuad(glow, viewMin.X, viewMin.Y, viewMax.X, viewMax.Y, 1.0f, copyShader);
		}
		else
		{
			drawQuad(glow, viewMin.X, viewMin.Y, viewMax.X, viewMax.Y, TheGlobalData->m_bloomStrength, addShader);
		}

		rinfo.Camera.Apply();
	}

	DX8Wrapper::Set_Texture(0, nullptr);
	releaseDefaults();
}

// four diagonal taps a quarter bright each; the first replaces the target so no clear is needed
Bool W3DBloom::blurPass(TextureClass *source, TextureClass *target, Real offsetU, Real offsetV, Bool shrink)
{
	static const ShaderClass copyShader = makeQuadShader(FALSE);
	static const ShaderClass addShader = makeQuadShader(TRUE);
	static const Real corners[4][2] = { { -1.0f, -1.0f }, { 1.0f, -1.0f }, { -1.0f, 1.0f }, { 1.0f, 1.0f } };

	if (!setTarget(target))
	{
		return false;
	}

	// the shrink samples a square of the larger source, a blur pass runs along one axis of its own size
	if (shrink)
	{
		for (Int tap = 0; tap < 4; ++tap)
		{
			const Real du = corners[tap][0] * offsetU;
			const Real dv = corners[tap][1] * offsetV;
			drawQuad(source, du, dv, 1.0f + du, 1.0f + dv, 0.25f, tap == 0 ? copyShader : addShader);
		}
		return true;
	}

	// the centre tap first so it replaces the target, then the pairs either side of it
	drawQuad(source, 0.0f, 0.0f, 1.0f, 1.0f, BLOOM_TAP_WEIGHT[0], copyShader);
	for (Int tap = 1; tap < 3; ++tap)
	{
		const Real du = offsetU * BLOOM_TAP_OFFSET[tap];
		const Real dv = offsetV * BLOOM_TAP_OFFSET[tap];
		drawQuad(source, du, dv, 1.0f + du, 1.0f + dv, BLOOM_TAP_WEIGHT[tap], addShader);
		drawQuad(source, -du, -dv, 1.0f - du, 1.0f - dv, BLOOM_TAP_WEIGHT[tap], addShader);
	}
	return true;
}

// fills the current viewport with the source, in clip space so no half pixel shift is needed
void W3DBloom::drawQuad(TextureClass *source, Real u0, Real v0, Real u1, Real v1, Real brightness, const ShaderClass &shader)
{
	static const Real cornerX[4] = { -1.0f, 1.0f, -1.0f, 1.0f };
	static const Real cornerY[4] = { 1.0f, 1.0f, -1.0f, -1.0f };
	const unsigned color = DX8Wrapper::Convert_Color_Clamp(Vector4(brightness, brightness, brightness, 1.0f));

	DynamicVBAccessClass vb(BUFFER_TYPE_DYNAMIC_DX8, dynamic_fvf_type, 4);
	{
		DynamicVBAccessClass::WriteLockClass lock(&vb);
		VertexFormatXYZNDUV2 *verts = lock.Get_Formatted_Vertex_Array();
		for (Int i = 0; i < 4; ++i)
		{
			verts[i].x = cornerX[i];
			verts[i].y = cornerY[i];
			verts[i].z = 0.0f;
			verts[i].nx = 0.0f;
			verts[i].ny = 0.0f;
			verts[i].nz = 0.0f;
			verts[i].diffuse = color;
			verts[i].u1 = (i & 1) ? u1 : u0;
			verts[i].v1 = (i & 2) ? v1 : v0;
			verts[i].u2 = 0.0f;
			verts[i].v2 = 0.0f;
		}
	}

	DynamicIBAccessClass ib(BUFFER_TYPE_DYNAMIC_DX8, 6);
	{
		DynamicIBAccessClass::WriteLockClass lock(&ib);
		unsigned short *inds = lock.Get_Index_Array();
		inds[0] = 0;
		inds[1] = 1;
		inds[2] = 2;
		inds[3] = 2;
		inds[4] = 1;
		inds[5] = 3;
	}

	DX8Wrapper::Set_Index_Buffer(ib, 0);
	DX8Wrapper::Set_Vertex_Buffer(vb);
	DX8Wrapper::Set_Texture(0, source);
	DX8Wrapper::Set_Shader(shader);
	DX8Wrapper::Draw_Triangles(0, 2, 0, 4);
}
