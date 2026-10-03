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

// FILE: MapPreview.h /////////////////////////////////////////////////
//-----------------------------------------------------------------------------
//
//                       Electronic Arts Pacific.
//
//                       Confidential Information
//                Copyright (C) 2002 - All Rights Reserved
//
//-----------------------------------------------------------------------------
//
//	created:	Oct 2002
//
//	Filename: 	MapPreview.h
//
//	author:		Chris Huybregts
//
//	purpose:
//
//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>

//-----------------------------------------------------------------------------
// SYSTEM INCLUDES ////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// USER INCLUDES //////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// FORWARD REFERENCES /////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// TYPE DEFINES ///////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
enum
{
	MAP_PREVIEW_HEIGHT = 128,//256,
	MAP_PREVIEW_WIDTH = 128,//256,
	HQ_PREVIEW_SIZE = 256,
	HQ_SUPERSAMPLE = 4,
};

class WbView3d;

/// Shading for the HQ preview. Colours are RGB, 0 to 255.
struct HQPreviewParams
{
	Real relief;		///< hillshade strength
	Real elevation;		///< brightness spread from the lowest to the highest ground
	Real waterFalloff;	///< depth in world units at which water reaches most of its deep colour
	Int shallow[3];
	Int deep[3];
};

class MapPreview
{
public:
	MapPreview();
	void save( CString mapName );

	static void getDefaultHQParams( HQPreviewParams *params );
	/// Renders the map from above and caches what composeHQ needs.
	Bool prepareHQ( WbView3d *view );
	/// Shades the cached render into HQ_PREVIEW_SIZE^2 BGRA pixels, top row north.
	void composeHQ( const HQPreviewParams &params, UnsignedByte *bgra );
	/// Writes the pixels as <map>.tga.
	static Bool writeHQ( CString mapName, const UnsignedByte *bgra );
private:
	void interpolateColorForHeight( RGBColor *color, Real height, Real hiZ, Real midZ, Real loZ );
	Bool mapPreviewToWorld(const ICoord2D *radar, Coord3D *world);
	void buildMapPreviewTexture( CString tgaName );
	void buildMapPreviewTextureAnime( CString tgaName );
	
	UnsignedInt m_pixelBuffer[MAP_PREVIEW_HEIGHT][MAP_PREVIEW_WIDTH];

	// Per supersampled pixel. A negative light marks a bridge or object over water, left unshaded.
	std::vector<UnsignedByte> m_hqScene;
	std::vector<Real> m_hqLight;
	std::vector<Real> m_hqHeight;
	std::vector<Real> m_hqDepth;


};
//-----------------------------------------------------------------------------
// INLINING ///////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// EXTERNALS //////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
