// WBQtHQPreviewBridge.cpp -- the MFC side of the Qt HQ map preview dialog. See WBQtHQPreviewBridge.h.
#include "StdAfx.h"

#ifdef RTS_HAS_QT

#include "Lib/BaseType.h"
#include "WorldBuilderDoc.h"
#include "wbview3d.h"
#include "MapPreview.h"
#include "qt/panels/WBQtHQPreviewBridge.h"

#include <stdio.h>

static const char *HQ_PREVIEW_SECTION = "HQMapPreview";

static MapPreview *s_preview = NULL;
static CString s_mapPath;

static void toParams(const WBQtHQPreviewParams *in, HQPreviewParams *out)
{
	out->relief = in->relief;
	out->elevation = in->elevation;
	out->waterFalloff = in->waterFalloff;
	for (Int k = 0; k < 3; k++)
	{
		out->shallow[k] = in->shallow[k];
		out->deep[k] = in->deep[k];
	}
}

static Real getProfileReal(const char *key, Real fallback)
{
	CString text = ::AfxGetApp()->GetProfileString(HQ_PREVIEW_SECTION, key, "");
	float value = 0.0f;
	return (!text.IsEmpty() && sscanf(text, "%f", &value) == 1) ? value : fallback;
}

static void writeProfileReal(const char *key, Real value)
{
	CString text;
	text.Format("%g", value);
	::AfxGetApp()->WriteProfileString(HQ_PREVIEW_SECTION, key, text);
}

// Colours are stored as 0xRRGGBB.
static void getProfileColor(const char *key, Int rgb[3])
{
	const Int packed = ::AfxGetApp()->GetProfileInt(HQ_PREVIEW_SECTION, key, -1);
	if (packed >= 0)
	{
		rgb[0] = (packed >> 16) & 0xff;
		rgb[1] = (packed >> 8) & 0xff;
		rgb[2] = packed & 0xff;
	}
}

static void writeProfileColor(const char *key, const int rgb[3])
{
	::AfxGetApp()->WriteProfileInt(HQ_PREVIEW_SECTION, key, (rgb[0] << 16) | (rgb[1] << 8) | rgb[2]);
}

int WBQtHQPreview_Size(void)
{
	return HQ_PREVIEW_SIZE;
}

void WBQtHQPreview_GetDefaults(WBQtHQPreviewParams *out)
{
	HQPreviewParams params;
	MapPreview::getDefaultHQParams(&params);
	out->relief = params.relief;
	out->elevation = params.elevation;
	out->waterFalloff = params.waterFalloff;
	for (Int k = 0; k < 3; k++)
	{
		out->shallow[k] = params.shallow[k];
		out->deep[k] = params.deep[k];
	}
}

void WBQtHQPreview_GetLast(WBQtHQPreviewParams *out)
{
	WBQtHQPreview_GetDefaults(out);
	out->relief = getProfileReal("Relief", out->relief);
	out->elevation = getProfileReal("Elevation", out->elevation);
	out->waterFalloff = getProfileReal("WaterFalloff", out->waterFalloff);
	getProfileColor("ShallowWater", out->shallow);
	getProfileColor("DeepWater", out->deep);
}

void WBQtHQPreview_Compose(const WBQtHQPreviewParams *params, unsigned char *bgra)
{
	if (s_preview == NULL || params == NULL || bgra == NULL)
	{
		return;
	}
	HQPreviewParams p;
	toParams(params, &p);
	s_preview->composeHQ(p, bgra);
}

int WBQtHQPreview_Save(const WBQtHQPreviewParams *params)
{
	if (s_preview == NULL || params == NULL)
	{
		return 0;
	}
	std::vector<UnsignedByte> pixels(HQ_PREVIEW_SIZE*HQ_PREVIEW_SIZE*4);
	WBQtHQPreview_Compose(params, &pixels[0]);
	if (!MapPreview::writeHQ(s_mapPath, &pixels[0]))
	{
		return 0;
	}
	writeProfileReal("Relief", params->relief);
	writeProfileReal("Elevation", params->elevation);
	writeProfileReal("WaterFalloff", params->waterFalloff);
	writeProfileColor("ShallowWater", params->shallow);
	writeProfileColor("DeepWater", params->deep);
	return 1;
}

int WBQtHQPreview_Run(void *view, const char *mapPath)
{
	MapPreview preview;
	if (view == NULL || mapPath == NULL || !preview.prepareHQ((WbView3d *)view))
	{
		return -1;
	}
	CString tgaPath = mapPath;
	tgaPath.Replace(".map", ".tga");

	s_preview = &preview;
	s_mapPath = mapPath;
	const int saved = WBQtHQPreview_Show(tgaPath);
	s_preview = NULL;
	return saved;
}

#endif // RTS_HAS_QT
