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

// W3DGroundNoise.cpp /////////////////////////////////////////////////////////////////////////////
// Builds the texture the Direct3D 9 ground shaders vary the terrain's light and colour with
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "Lib/BaseType.h"
#include "WWLib/always.h"
#include "W3DDevice/GameClient/W3DGroundNoise.h"
#include "W3DDevice/GameClient/W3DNoiseTexture.h"
#include "Common/GlobalData.h"
#include "WW3D2/dx8wrapper.h"
#include "WW3D2/formconv.h"
#include "WW3D2/texture.h"

#include <vector>

static const Int NOISE_SIZE = 512;

// Lattice cells across the texture, octaves, each octave's gain, how hard tails squeeze into patches, and the share of the strength.
struct GroundNoiseLayer
{
	Int cells;
	Int octaves;
	Real gain;
	Real squeeze;
	Real share;
};

// Red, green and blue shade broadest first, green and blue read shrunk and turned, and alpha tints.
static const GroundNoiseLayer Layers[4] =
{
	{ 4, 5, 0.5f, 0.0f, 0.45f },
	{ 4, 5, 0.6f, 0.9f, 0.35f },
	{ 4, 6, 0.7f, 0.9f, 0.5f },
	{ 2, 3, 0.5f, 0.0f, 1.0f }
};

// groundnoise.hlsli subtracts this from the three shading channels' sum.
static const Real SHADE_OFFSET = 1.0f;

static TextureClass *GroundNoiseTexture = nullptr;
static TextureClass *WhiteTexture = nullptr;
static Bool GroundNoiseFailed = FALSE;
static Real BuiltStrength = 0.0f;
static Real BuiltTint = 0.0f;
static Real BuiltBrightness = 0.0f;

static TextureClass *Build_Ground_Noise(Real strength, Real tint, Real brightness)
{
#if defined(BUILD_WITH_D3D9)
	std::vector<UnsignedInt> pixels(NOISE_SIZE * NOISE_SIZE, 0);
	std::vector<Real> field(NOISE_SIZE * NOISE_SIZE);

	// The shading channels' means sum to the brightness once groundnoise.hlsli takes its offset off.
	const Real shadeMean = (brightness + SHADE_OFFSET) / 3.0f;
	static const Int shifts[4] = { 16, 8, 0, 24 };
	for (Int channel = 0; channel < 4; channel++)
	{
		const GroundNoiseLayer &layer = Layers[channel];
		W3DNoiseTexture::buildLayer(&field[0], NOISE_SIZE, layer.cells, layer.octaves, layer.gain, layer.squeeze, channel + 1);

		const Real mean = (channel < 3) ? shadeMean : 0.5f;
		const Real amplitude = (channel < 3) ? strength * layer.share : tint;
		for (Int i = 0; i < NOISE_SIZE * NOISE_SIZE; i++)
		{
			pixels[i] |= W3DNoiseTexture::toByte(mean + amplitude * field[i]) << shifts[channel];
		}
	}

	return W3DNoiseTexture::createTexture(&pixels[0], NOISE_SIZE);
#else
	(void)strength;
	(void)tint;
	(void)brightness;
	return nullptr;
#endif
}

TextureClass *W3DGroundNoise::getTexture()
{
	const Real strength = max(TheGlobalData->m_groundNoiseStrength, 0.0f);
	const Real tint = max(TheGlobalData->m_groundNoiseTint, 0.0f);
	const Real brightness = max(TheGlobalData->m_groundNoiseBrightness, 0.0f);

	// A live GameData reload changes the settings under a built texture.
	if (GroundNoiseTexture != nullptr && (strength != BuiltStrength || tint != BuiltTint || brightness != BuiltBrightness))
	{
		REF_PTR_RELEASE(GroundNoiseTexture);
	}

	if (GroundNoiseTexture == nullptr && !GroundNoiseFailed)
	{
		GroundNoiseTexture = Build_Ground_Noise(strength, tint, brightness);
		GroundNoiseFailed = (GroundNoiseTexture == nullptr);
		BuiltStrength = strength;
		BuiltTint = tint;
		BuiltBrightness = brightness;
	}
	return GroundNoiseTexture;
}

TextureClass *W3DGroundNoise::getWhiteTexture()
{
#if defined(BUILD_WITH_D3D9)
	if (WhiteTexture == nullptr)
	{
		IDirect3DTexture8 *texture = DX8Wrapper::_Create_DX8_Texture(1, 1, WW3D_FORMAT_A8R8G8B8, MIP_LEVELS_1, D3DPOOL_MANAGED, false);
		if (texture == nullptr)
		{
			return nullptr;
		}

		IDirect3DTexture8 *lockable = DX8Wrapper::_Peek_Lockable_Texture(texture);
		D3DLOCKED_RECT locked;
		if (FAILED(lockable->LockRect(0, &locked, nullptr, 0)))
		{
			texture->Release();
			return nullptr;
		}
		*(UnsignedInt *)locked.pBits = 0xffffffffu;
		lockable->UnlockRect(0);
		DX8Wrapper::_Upload_Lockable_Texture(texture);
		WhiteTexture = W3DNoiseTexture::wrapTexture(texture);
	}
#endif
	return WhiteTexture;
}

void W3DGroundNoise::setupStage(Int stage)
{
	// Camera space back to the world, then to texture tiles. GroundNoiseSize spans one of red's cells.
	D3DMATRIX view;
	D3DMATRIX inverseView;
	float det;
	DX8Wrapper::_Get_DX8_Transform(D3DTS_VIEW, view);
	Invert_D3DMATRIX(inverseView, &det, view);

	const Real scale = 1.0f / (Layers[0].cells * max(TheGlobalData->m_groundNoiseSize, 1.0f));
	D3DMATRIX toTexture;
	Set_D3DMATRIX_Scaling(toTexture, scale, scale, 1.0f);

	DX8Wrapper::_Set_DX8_Transform((D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0 + stage), inverseView * toTexture);
	DX8Wrapper::Set_DX8_Texture_Stage_State(stage, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEPOSITION);
	DX8Wrapper::Set_DX8_Texture_Stage_State(stage, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
	DX8Wrapper::Set_DX8_Texture_Stage_State(stage, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
	DX8Wrapper::Set_DX8_Texture_Stage_State(stage, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
	DX8Wrapper::Set_DX8_Texture_Stage_State(stage, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
	DX8Wrapper::Set_DX8_Texture_Stage_State(stage, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
	DX8Wrapper::Set_DX8_Texture_Stage_State(stage, D3DTSS_MIPFILTER, D3DTEXF_LINEAR);
}

void W3DGroundNoise::releaseResources()
{
	REF_PTR_RELEASE(GroundNoiseTexture);
	REF_PTR_RELEASE(WhiteTexture);
	GroundNoiseFailed = FALSE;
}
