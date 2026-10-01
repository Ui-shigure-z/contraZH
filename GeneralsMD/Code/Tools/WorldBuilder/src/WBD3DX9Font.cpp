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

#include "StdAfx.h"
#include "WBD3DX9Font.h"

#if defined(BUILD_WITH_D3D9)

typedef HRESULT (WINAPI *D3DXCreateFontAType)(IDirect3DDevice9 *device, INT height, UINT width,
	UINT weight, UINT mipLevels, BOOL italic, DWORD charSet, DWORD outputPrecision, DWORD quality,
	DWORD pitchAndFamily, LPCSTR faceName, ID3DXFont **font);
typedef HRESULT (WINAPI *D3DXCreateSpriteType)(IDirect3DDevice9 *device, ID3DXSprite **sprite);

static HMODULE findD3DX9()
{
	static HMODULE s_dll = NULL;
	static Bool s_tried = false;
	if (s_tried) {
		return s_dll;
	}
	s_tried = true;

	// The DirectX end-user runtime ships these; newest first.
	static const char *const dllNames[] = {
		"d3dx9_43.dll", "d3dx9_42.dll", "d3dx9_41.dll", "d3dx9_40.dll", "d3dx9_39.dll",
		"d3dx9_38.dll", "d3dx9_37.dll", "d3dx9_36.dll", "d3dx9_35.dll", "d3dx9_34.dll",
		"d3dx9_33.dll", "d3dx9_32.dll", "d3dx9_31.dll", "d3dx9_30.dll", "d3dx9_29.dll",
		"d3dx9_28.dll", "d3dx9_27.dll", "d3dx9_26.dll", "d3dx9_25.dll", "d3dx9_24.dll",
	};
	for (size_t i = 0; i < sizeof(dllNames) / sizeof(dllNames[0]); ++i) {
		HMODULE dll = ::LoadLibraryA(dllNames[i]);
		if (dll == NULL) {
			continue;
		}
		if (::GetProcAddress(dll, "D3DXCreateFontA") != NULL) {
			DEBUG_LOG(("WBD3DX9Font: using %s", dllNames[i]));
			s_dll = dll;
			return s_dll;
		}
		::FreeLibrary(dll);
	}
	DEBUG_LOG(("WBD3DX9Font: no d3dx9 runtime found; the Old (D3DX) label renderer is unavailable"));
	return NULL;
}

ID3DXFont *WBD3DX9CreateFont(IDirect3DDevice9 *device, const LOGFONTA &logFont)
{
	HMODULE dll = findD3DX9();
	if (dll == NULL || device == NULL) {
		return NULL;
	}
	D3DXCreateFontAType create = (D3DXCreateFontAType)::GetProcAddress(dll, "D3DXCreateFontA");
	if (create == NULL) {
		return NULL;
	}
	ID3DXFont *font = NULL;
	HRESULT hr = create(device, logFont.lfHeight, logFont.lfWidth, logFont.lfWeight, 1,
		logFont.lfItalic, logFont.lfCharSet, logFont.lfOutPrecision, logFont.lfQuality,
		logFont.lfPitchAndFamily, logFont.lfFaceName, &font);
	if (FAILED(hr)) {
		DEBUG_LOG(("WBD3DX9Font: D3DXCreateFontA failed 0x%08X", hr));
		return NULL;
	}
	return font;
}

ID3DXSprite *WBD3DX9CreateSprite(IDirect3DDevice9 *device)
{
	HMODULE dll = findD3DX9();
	if (dll == NULL || device == NULL) {
		return NULL;
	}
	D3DXCreateSpriteType create = (D3DXCreateSpriteType)::GetProcAddress(dll, "D3DXCreateSprite");
	if (create == NULL) {
		return NULL;
	}
	ID3DXSprite *sprite = NULL;
	HRESULT hr = create(device, &sprite);
	if (FAILED(hr)) {
		DEBUG_LOG(("WBD3DX9Font: D3DXCreateSprite failed 0x%08X", hr));
		return NULL;
	}
	return sprite;
}

#endif // BUILD_WITH_D3D9
