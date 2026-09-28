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

// FILE: W3DNoiseTexture.h ////////////////////////////////////////////////////////////////////////

#pragma once

#include "Lib/BaseType.h"
#include "WW3D2/dx8compat.h"

class TextureClass;

// Tiling gradient noise baked into textures for the Direct3D 9 ground and sky shaders.
class W3DNoiseTexture
{
public:
	/// Fills a size by size field with octaves of noise that tile across it, at a mean of 0 and a standard deviation of 1.
	/// Cells is the first octave's lattice cells along a side, each octave doubles them and scales by gain, and squeeze bunches tails into patches.
	static void buildLayer(Real *field, Int size, Int cells, Int octaves, Real gain, Real squeeze, Int seed);

	static UnsignedInt toByte(Real value);	///< 0-1 to a byte, clamped

	/// A repeating, mipmapped A8R8G8B8 texture of size by size pixels, or null when it cannot be made.
	static TextureClass *createTexture(const UnsignedInt *pixels, Int size);

	/// Hands a texture made here to a TextureClass, which the wrapper's deferred texture binding takes.
	static TextureClass *wrapTexture(IDirect3DTexture8 *texture);
};
