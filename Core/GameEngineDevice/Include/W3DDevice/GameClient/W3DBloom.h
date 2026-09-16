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

// FILE: W3DBloom.h /////////////////////////////////////////////////////////

#pragma once

#include "Lib/BaseType.h"
#include "WW3D2/ww3dformat.h"

class TextureClass;
class RenderInfoClass;
class ShaderClass;
struct IDirect3DSurface8;

// Adds a soft glow around additive particles and additive meshes. The caller draws the particles a
// second time into the bloom target between begin and end; end replays the meshes the DX8 mesh
// renderer kept, blurs the target and adds it onto the back buffer.
class W3DBloom
{
public:
	W3DBloom();
	~W3DBloom();

	Bool begin(RenderInfoClass &rinfo);	///< true when the caller should draw the additive pass now
	void end(RenderInfoClass &rinfo);
	void ReleaseResources();						///< drops the targets before a device reset; begin recreates them

private:
	Bool acquireTargets(Int width, Int height, WW3DFormat format);
	void releaseTargets();
	void releaseDefaults();
	Bool setTarget(TextureClass *target);
	Bool blurPass(TextureClass *source, TextureClass *target, Real offsetU, Real offsetV, Bool shrink);
	void drawQuad(TextureClass *source, Real u0, Real v0, Real u1, Real v1, Real brightness, const ShaderClass &shader);

	TextureClass *m_fullTarget;					///< screen sized, receives the additive particles
	TextureClass *m_blurTarget[2];			///< reduced size, ping-pong between blur passes
	IDirect3DSurface8 *m_defaultTarget;	///< the back buffer, held only between begin and end
	IDirect3DSurface8 *m_defaultDepth;
	Int m_width;
	Int m_height;
	Bool m_disabled;										///< the device refused the target; stays set until ReleaseResources
};

extern W3DBloom *TheW3DBloom;
