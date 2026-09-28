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

// W3DNoiseTexture.cpp ////////////////////////////////////////////////////////////////////////////
// Tiling gradient noise for the textures the Direct3D 9 ground and sky shaders read
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "Lib/BaseType.h"
#include "WWLib/always.h"
#include "W3DDevice/GameClient/W3DNoiseTexture.h"
#include "WW3D2/dx8wrapper.h"
#include "WW3D2/formconv.h"
#include "WW3D2/texture.h"

#include <math.h>
#include <vector>

static UnsignedInt Hash_Lattice(Int x, Int y, Int seed)
{
	UnsignedInt h = (UnsignedInt)x * 374761393u + (UnsignedInt)y * 668265263u + (UnsignedInt)seed * 2246822519u;
	h = (h ^ (h >> 13)) * 1274126177u;
	return h ^ (h >> 16);
}

static Real Fade(Real t)
{
	return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

// Adds one octave of gradient noise that tiles across the field, with this many cells along each side.
static void Add_Gradient_Noise(Real *field, Int size, Int cells, Int seed, Real amplitude)
{
	std::vector<Real> gradients(cells * cells * 2);
	for (Int i = 0; i < cells * cells; i++)
	{
		const Real angle = (Real)(Hash_Lattice(i % cells, i / cells, seed) & 0xffff) * (6.2831853f / 65536.0f);
		gradients[i * 2] = (Real)cos(angle);
		gradients[i * 2 + 1] = (Real)sin(angle);
	}

	const Real step = (Real)cells / size;
	for (Int y = 0; y < size; y++)
	{
		const Real fy = (y + 0.5f) * step;
		const Int y0 = (Int)fy;
		const Int y1 = (y0 + 1) % cells;
		const Real ty = fy - y0;
		const Real v = Fade(ty);
		for (Int x = 0; x < size; x++)
		{
			const Real fx = (x + 0.5f) * step;
			const Int x0 = (Int)fx;
			const Int x1 = (x0 + 1) % cells;
			const Real tx = fx - x0;
			const Real u = Fade(tx);

			const Real *g00 = &gradients[(y0 * cells + x0) * 2];
			const Real *g10 = &gradients[(y0 * cells + x1) * 2];
			const Real *g01 = &gradients[(y1 * cells + x0) * 2];
			const Real *g11 = &gradients[(y1 * cells + x1) * 2];
			const Real n00 = g00[0] * tx + g00[1] * ty;
			const Real n10 = g10[0] * (tx - 1.0f) + g10[1] * ty;
			const Real n01 = g01[0] * tx + g01[1] * (ty - 1.0f);
			const Real n11 = g11[0] * (tx - 1.0f) + g11[1] * (ty - 1.0f);

			const Real top = n00 + (n10 - n00) * u;
			const Real bottom = n01 + (n11 - n01) * u;
			field[y * size + x] += amplitude * (top + (bottom - top) * v);
		}
	}
}

// Shifts and scales the field to a mean of 0 and a standard deviation of 1.
static void Normalize(Real *field, Int size)
{
	const Int count = size * size;
	double sum = 0.0;
	double sumSquares = 0.0;
	for (Int i = 0; i < count; i++)
	{
		sum += field[i];
		sumSquares += (double)field[i] * field[i];
	}
	const double mean = sum / count;
	const double variance = sumSquares / count - mean * mean;
	const Real scale = (variance > 0.0) ? (Real)(1.0 / sqrt(variance)) : 0.0f;
	for (Int i = 0; i < count; i++)
	{
		field[i] = (field[i] - (Real)mean) * scale;
	}
}

void W3DNoiseTexture::buildLayer(Real *field, Int size, Int cells, Int octaves, Real gain, Real squeeze, Int seed)
{
	memset(field, 0, sizeof(Real) * size * size);
	Real amplitude = 1.0f;
	for (Int octave = 0; octave < octaves; octave++)
	{
		Add_Gradient_Noise(field, size, cells << octave, seed * 16 + octave, amplitude);
		amplitude *= gain;
	}
	Normalize(field, size);

	if (squeeze > 0.0f)
	{
		for (Int i = 0; i < size * size; i++)
		{
			field[i] = (Real)tanh(squeeze * field[i]);
		}
		Normalize(field, size);
	}
}

UnsignedInt W3DNoiseTexture::toByte(Real value)
{
	return (UnsignedInt)(clamp(0.0f, value, 1.0f) * 255.0f + 0.5f);
}

TextureClass *W3DNoiseTexture::wrapTexture(IDirect3DTexture8 *texture)
{
	TextureClass *wrapped = NEW_REF(TextureClass, (texture));
	texture->Release();
	return wrapped;
}

TextureClass *W3DNoiseTexture::createTexture(const UnsignedInt *pixels, Int size)
{
#if defined(BUILD_WITH_D3D9)
	IDirect3DTexture8 *texture = DX8Wrapper::_Create_DX8_Texture(size, size, WW3D_FORMAT_A8R8G8B8, MIP_LEVELS_ALL, D3DPOOL_MANAGED, false);
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
	for (Int y = 0; y < size; y++)
	{
		memcpy((UnsignedByte *)locked.pBits + y * locked.Pitch, &pixels[y * size], size * sizeof(UnsignedInt));
	}
	lockable->UnlockRect(0);

	// Box-filtered mips keep distant reads from shimmering.
	Filter_Texture_Mipmaps(lockable);
	DX8Wrapper::_Upload_Lockable_Texture(texture);

	TextureClass *wrapped = wrapTexture(texture);
	wrapped->Get_Filter().Set_Min_Filter(TextureFilterClass::FILTER_TYPE_BEST);
	wrapped->Get_Filter().Set_Mag_Filter(TextureFilterClass::FILTER_TYPE_BEST);
	wrapped->Get_Filter().Set_Mip_Mapping(TextureFilterClass::FILTER_TYPE_BEST);
	wrapped->Get_Filter().Set_U_Addr_Mode(TextureFilterClass::TEXTURE_ADDRESS_REPEAT);
	wrapped->Get_Filter().Set_V_Addr_Mode(TextureFilterClass::TEXTURE_ADDRESS_REPEAT);
	return wrapped;
#else
	(void)pixels;
	(void)size;
	return nullptr;
#endif
}
