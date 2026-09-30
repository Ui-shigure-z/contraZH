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

// WBPngTexture.cpp -- PNG -> TextureClass for the D3D9 build (no D3DX there).

#include "StdAfx.h"
#include "WBPngTexture.h"

#if defined(BUILD_WITH_D3D9)

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_FAILURE_STRINGS
#include <stb_image.h>

#include "WW3D2/texture.h"
#include "WW3D2/surfaceclass.h"
#include "WW3D2/ww3dformat.h"

// Halves an A8R8G8B8 image: nearest keeps the top-left sample of each 2x2 block
// (D3DX_FILTER_POINT), otherwise the block is averaged (D3DX_FILTER_LINEAR).
static void downsampleLevel(const UnsignedInt *src, Int srcW, Int srcH,
														UnsignedInt *dst, Int dstW, Int dstH, Bool nearest)
{
	for (Int y = 0; y < dstH; ++y)
	{
		const Int sy0 = min(y * 2, srcH - 1);
		const Int sy1 = min(y * 2 + 1, srcH - 1);
		for (Int x = 0; x < dstW; ++x)
		{
			const Int sx0 = min(x * 2, srcW - 1);
			const Int sx1 = min(x * 2 + 1, srcW - 1);
			if (nearest)
			{
				dst[y * dstW + x] = src[sy0 * srcW + sx0];
				continue;
			}
			const UnsignedInt p[4] =
			{
				src[sy0 * srcW + sx0], src[sy0 * srcW + sx1],
				src[sy1 * srcW + sx0], src[sy1 * srcW + sx1],
			};
			UnsignedInt out = 0;
			for (Int shift = 0; shift < 32; shift += 8)
			{
				UnsignedInt sum = 0;
				for (Int i = 0; i < 4; ++i)
				{
					sum += (p[i] >> shift) & 0xFF;
				}
				out |= ((sum + 2) / 4) << shift;
			}
			dst[y * dstW + x] = out;
		}
	}
}

TextureClass *WBPngTexture_Load(const char *path, Bool nearestFilter)
{
	if (path == NULL || path[0] == 0)
	{
		return NULL;
	}

	int w = 0, h = 0, comp = 0;
	unsigned char *rgba = stbi_load(path, &w, &h, &comp, 4);
	if (rgba == NULL || w <= 0 || h <= 0)
	{
		if (rgba != NULL)
		{
			stbi_image_free(rgba);
		}
		return NULL;
	}

	TextureClass *tex = new TextureClass((unsigned)w, (unsigned)h, WW3D_FORMAT_A8R8G8B8,
		MIP_LEVELS_ALL, TextureClass::POOL_MANAGED, false, false);

	// Level 0: stb hands back RGBA bytes; the texture wants A8R8G8B8 (BGRA in memory).
	UnsignedInt *cur = new UnsignedInt[w * h];
	for (Int i = 0; i < w * h; ++i)
	{
		const unsigned char *px = rgba + i * 4;
		cur[i] = ((UnsignedInt)px[3] << 24) | ((UnsignedInt)px[0] << 16) |
						 ((UnsignedInt)px[1] << 8) | (UnsignedInt)px[2];
	}
	stbi_image_free(rgba);

	Int curW = w, curH = h;
	const Int levels = (Int)tex->Get_Mip_Level_Count();
	for (Int level = 0; level < levels; ++level)
	{
		SurfaceClass *surf = tex->Get_Surface_Level(level);
		if (surf != NULL)
		{
			SurfaceClass::SurfaceDescription desc;
			surf->Get_Description(desc);
			int pitch = 0;
			unsigned char *bits = (unsigned char *)surf->Lock(&pitch);
			if (bits != NULL)
			{
				const Int copyW = min(curW, (Int)desc.Width);
				const Int copyH = min(curH, (Int)desc.Height);
				for (Int y = 0; y < copyH; ++y)
				{
					memcpy(bits + y * pitch, cur + y * curW, copyW * sizeof(UnsignedInt));
				}
				surf->Unlock();
			}
			REF_PTR_RELEASE(surf);
		}
		if (level + 1 < levels)
		{
			const Int nextW = max(1, curW / 2);
			const Int nextH = max(1, curH / 2);
			UnsignedInt *next = new UnsignedInt[nextW * nextH];
			downsampleLevel(cur, curW, curH, next, nextW, nextH, nearestFilter);
			delete [] cur;
			cur = next;
			curW = nextW;
			curH = nextH;
		}
	}
	delete [] cur;
	return tex;
}

#endif // BUILD_WITH_D3D9
