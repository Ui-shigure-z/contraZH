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

// FILE: W3DSkyClouds.h ///////////////////////////////////////////////////////////////////////////

#pragma once

#include "Lib/BaseType.h"
#include "WW3D2/dx8compat.h"

class RenderInfoClass;
class TextureClass;
class BaseHeightMapRenderObjClass;

// The HQ sky's cloud shadows. Each frame they are drawn into a map over the ground the camera sees,
// which every surface reads in place of the legacy scrolling cloud texture.
class W3DSkyClouds
{
public:
	W3DSkyClouds();
	~W3DSkyClouds();

	/// Draws this frame's map around what the camera sees, once a frame, while the HQ sky is on.
	void update(RenderInfoClass &rinfo, BaseHeightMapRenderObjClass &terrain);

	/// True while the map is current, so the cloud map readers take it and its matrix.
	Bool isActive() const;

	TextureClass *getTexture() const { return m_target; }

	/// Camera space to the map's texcoords, for camera-space position texgen.
	void getTextureMatrix(D3DMATRIX &dest, const D3DMATRIX &inverseView) const;

	void ReleaseResources();	///< drops the shader, noise and map before a device reset; the next update makes them again

private:
	enum { READ_WARP, READ_SHAPE_A, READ_SHAPE_B, READ_DETAIL, READ_COUNT };

	Bool acquire();
	void advance(Real seconds);

	TextureClass *m_target;
	TextureClass *m_noise;
	DWORD m_shader;
	Bool m_failed;				///< a resource could not be made, so the legacy clouds stay until the next reset
	Int m_mapSize;
	UnsignedInt m_updateFrame;	///< the frame update last ran in
	UnsignedInt m_drawnFrame;	///< the frame the map was last drawn in
	Bool m_drawn;
	Real m_originX;				///< the drawn map's world corner and width
	Real m_originY;
	Real m_span;
	double m_travel[READ_COUNT][2];	///< how far each read has drifted, in its own noise texcoords, wrapped to 0-1
};

extern W3DSkyClouds *TheW3DSkyClouds;
