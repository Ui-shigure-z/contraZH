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

// The D3D9 backend is built without the D3DX SDK, so the Old (D3DX) label
// renderer gets its font by loading d3dx9_43.dll at runtime. The interface below
// mirrors ID3DXFont from d3dx9core.h (June 2010 SDK) so the vtable matches.

#ifndef __WB_D3DX9_FONT_H_
#define __WB_D3DX9_FONT_H_

#if defined(BUILD_WITH_D3D9)

#include <windows.h>
#include <unknwn.h>

struct IDirect3DDevice9;
struct IDirect3DTexture9;
struct D3DXFONT_DESCA;
struct D3DXFONT_DESCW;
struct D3DXMATRIX;
struct D3DXVECTOR3;

#define WB_D3DXSPRITE_DONOTSAVESTATE (1 << 0)
#define WB_D3DXSPRITE_DONOTMODIFY_RENDERSTATE (1 << 1)
#define WB_D3DXSPRITE_ALPHABLEND (1 << 4)

DECLARE_INTERFACE_(ID3DXSprite, IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;

	STDMETHOD(GetDevice)(THIS_ IDirect3DDevice9 **ppDevice) PURE;
	STDMETHOD(GetTransform)(THIS_ D3DXMATRIX *pTransform) PURE;
	STDMETHOD(SetTransform)(THIS_ const D3DXMATRIX *pTransform) PURE;
	STDMETHOD(SetWorldViewRH)(THIS_ const D3DXMATRIX *pWorld, const D3DXMATRIX *pView) PURE;
	STDMETHOD(SetWorldViewLH)(THIS_ const D3DXMATRIX *pWorld, const D3DXMATRIX *pView) PURE;
	STDMETHOD(Begin)(THIS_ DWORD Flags) PURE;
	STDMETHOD(Draw)(THIS_ IDirect3DTexture9 *pTexture, const RECT *pSrcRect, const D3DXVECTOR3 *pCenter, const D3DXVECTOR3 *pPosition, DWORD Color) PURE;
	STDMETHOD(Flush)(THIS) PURE;
	STDMETHOD(End)(THIS) PURE;
	STDMETHOD(OnLostDevice)(THIS) PURE;
	STDMETHOD(OnResetDevice)(THIS) PURE;
};

DECLARE_INTERFACE_(ID3DXFont, IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID iid, LPVOID *ppv) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;

	STDMETHOD(GetDevice)(THIS_ IDirect3DDevice9 **ppDevice) PURE;
	STDMETHOD(GetDescA)(THIS_ D3DXFONT_DESCA *pDesc) PURE;
	STDMETHOD(GetDescW)(THIS_ D3DXFONT_DESCW *pDesc) PURE;
	STDMETHOD_(BOOL, GetTextMetricsA)(THIS_ TEXTMETRICA *pTextMetrics) PURE;
	STDMETHOD_(BOOL, GetTextMetricsW)(THIS_ TEXTMETRICW *pTextMetrics) PURE;
	STDMETHOD_(HDC, GetDC)(THIS) PURE;
	STDMETHOD(GetGlyphData)(THIS_ UINT Glyph, IDirect3DTexture9 **ppTexture, RECT *pBlackBox, POINT *pCellInc) PURE;
	STDMETHOD(PreloadCharacters)(THIS_ UINT First, UINT Last) PURE;
	STDMETHOD(PreloadGlyphs)(THIS_ UINT First, UINT Last) PURE;
	STDMETHOD(PreloadTextA)(THIS_ LPCSTR pString, INT Count) PURE;
	STDMETHOD(PreloadTextW)(THIS_ LPCWSTR pString, INT Count) PURE;
	STDMETHOD_(INT, DrawTextA)(THIS_ ID3DXSprite *pSprite, LPCSTR pString, INT Count, LPRECT pRect, DWORD Format, DWORD Color) PURE;
	STDMETHOD_(INT, DrawTextW)(THIS_ ID3DXSprite *pSprite, LPCWSTR pString, INT Count, LPRECT pRect, DWORD Format, DWORD Color) PURE;
	STDMETHOD(OnLostDevice)(THIS) PURE;
	STDMETHOD(OnResetDevice)(THIS) PURE;
};

// Wrap D3DXCreateFontA / D3DXCreateSprite from the runtime-loaded DLL. They return
// NULL when the DLL or the export is missing, so callers fall back to the atlas renderers.
ID3DXFont *WBD3DX9CreateFont(IDirect3DDevice9 *device, const LOGFONTA &logFont);
ID3DXSprite *WBD3DX9CreateSprite(IDirect3DDevice9 *device);

#endif // BUILD_WITH_D3D9

#endif // __WB_D3DX9_FONT_H_
