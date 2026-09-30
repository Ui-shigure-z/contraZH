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

#pragma once

#include "Lib/BaseType.h"

class TextureClass;

// Decodes a PNG with stb_image and uploads it as an A8R8G8B8 texture with a full mip
// chain (nearest or 2x2 box filtered). Returns NULL when the file cannot be read.
// The D3D9 backend has no D3DX, so this replaces D3DXCreateTextureFromFileEx for the
// tracing overlay; the D3D8 build keeps D3DX and never calls this.
TextureClass *WBPngTexture_Load(const char *path, Bool nearestFilter);
