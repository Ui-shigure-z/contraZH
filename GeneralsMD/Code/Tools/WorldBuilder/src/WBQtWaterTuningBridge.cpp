// WBQtWaterTuningBridge.cpp -- MFC side of the Qt Water tuning window.
//
// Holds the table of WaterTransparency keys the window shows, and writes each change into the
// map's override so the 3D view draws it. Whole body behind RTS_HAS_QT; empty TU when Qt is OFF.

#include "StdAfx.h"

#ifdef RTS_HAS_QT

#include "Lib/BaseType.h"
#include "GameClient/Water.h"
#include "W3DDevice/GameClient/BaseHeightMap.h"
#include "WorldBuilderDoc.h"
#include "wbview3d.h"
#include "qt/panels/WBQtWaterTuningBridge.h"

namespace
{
	struct WaterKey
	{
		const char *key;
		const char *help;
		int kind;
		float lo;
		float hi;
		float step;
		int advanced;
		Real WaterTransparencySetting::*real;
		Bool WaterTransparencySetting::*flag;
		RGBColor WaterTransparencySetting::*color;
		Bool rebuildsTerrain;	// the terrain caches this key in its shoreline tiles
	};

#define WATER_REAL(key, member, lo, hi, step, advanced, help) \
	{ key, help, WBQT_WATER_FLOAT, lo, hi, step, advanced, &WaterTransparencySetting::member, NULL, NULL, FALSE }
#define WATER_SHORE(key, member, lo, hi, step, advanced, help) \
	{ key, help, WBQT_WATER_FLOAT, lo, hi, step, advanced, &WaterTransparencySetting::member, NULL, NULL, TRUE }
#define WATER_BOOL(key, member, advanced, help) \
	{ key, help, WBQT_WATER_BOOL, 0.0f, 1.0f, 1.0f, advanced, NULL, &WaterTransparencySetting::member, NULL, FALSE }
#define WATER_COLOR(key, member, advanced, help) \
	{ key, help, WBQT_WATER_COLOR, 0.0f, 255.0f, 1.0f, advanced, NULL, NULL, &WaterTransparencySetting::member, FALSE }

	// IsWater, then the keys of the FX tuner's Water tab in its order, ranges and steps.
	const WaterKey s_keys[] =
	{
		WATER_BOOL("IsWater", m_isWater, 0,
			"No draws the old water without shaders, for lava and the like."),
		WATER_REAL("ShaderWaterOpacity", m_shaderWaterOpacity, 0.0f, 1.0f, 0.01f, 0,
			"Deep water opacity. 1 hides the seabed. 0 uses TransparentWaterMinOpacity."),
		WATER_REAL("ShaderWaterClarity", m_shaderWaterClarity, 0.1f, 10.0f, 0.1f, 0,
			"Scales TransparentWaterDepth. Higher sees deeper into the shallows."),
		WATER_COLOR("ShaderWaterDeepColor", m_shaderWaterDeepColor, 0,
			"Colour of deep water on shader model 3 cards. Unticked leaves the key out, so Water.ini decides."),
		WATER_REAL("ShaderWaterWaveScale", m_shaderWaterWaveScale, 1.0f, 2000.0f, 1.0f, 0,
			"World units one ripple pattern covers. Higher gives broader waves."),
		WATER_REAL("ShaderWaterWaveStrength", m_shaderWaterWaveStrength, 0.0f, 2.0f, 0.01f, 0,
			"Ripple steepness. Drives glint, reflection and bending."),
		WATER_REAL("ShaderWaterSpecular", m_shaderWaterSpecular, 0.0f, 5.0f, 0.05f, 0,
			"Scales the sun glint. 0 turns it off."),
		WATER_REAL("ShaderWaterSparkle", m_shaderWaterSparkle, 0.0f, 10.0f, 0.1f, 0,
			"Brightness of the small sun sparkles on the fine waves, on shader model 3 cards. 0 turns them off."),
		WATER_REAL("ShaderWaterReflection", m_shaderWaterReflection, 0.0f, 10.0f, 0.05f, 0,
			"Scales the sky reflection. 0 turns it off. All reflection caps at 80%."),
		WATER_REAL("ShaderWaterSwellHeight", m_shaderWaterSwellHeight, 0.0f, 10.0f, 0.1f, 0,
			"Height of the vertex waves on lakes and seas, in world units. 0 turns them off."),
		WATER_REAL("ShaderWaterFoamStrength", m_shaderWaterFoamStrength, 0.0f, 2.0f, 0.05f, 0,
			"Brightness of the foam. 0 turns it off."),
		WATER_BOOL("ShaderWaterAutoMeasure", m_shaderWaterAutoMeasure, 0,
			"Calms water by its distance from shore, so ponds, harbours and rivers are calmer than open sea."),
		WATER_REAL("ShaderWaterEnclosedCalm", m_shaderWaterEnclosedCalm, 0.0f, 1.0f, 0.01f, 0,
			"How much calmer enclosed water is than open water, 0 to 1: smaller waves, less swell and fewer sparkles."),
		WATER_BOOL("ShaderWaterZoomCompensation", m_shaderWaterZoomCompensation, 0,
			"Keeps the ripples and sun specks as they look up close at any zoom and camera pitch."),

		WATER_SHORE("TransparentWaterDepth", m_transparentWaterDepth, 0.0f, 50.0f, 0.1f, 1,
			"Depth over which the seabed fades out and the shore fades in. 0 gives a hard shore edge."),
		WATER_SHORE("TransparentWaterMinOpacity", m_minWaterOpacity, 0.0f, 1.0f, 0.01f, 1,
			"Deep water opacity of the old water, and of shader water when ShaderWaterOpacity is 0."),
		WATER_REAL("ShaderWaterFoamDepth", m_shaderWaterFoamDepth, 0.0f, 30.0f, 0.5f, 1,
			"Depth where shore foam fades out. 0 turns foam off, crest foam included."),
		WATER_REAL("ShaderWaterSpecularSpread", m_shaderWaterSpecularSpread, 0.1f, 16.0f, 0.1f, 1,
			"Widens the sun glint to more view angles. Below 1 narrows it."),
		WATER_BOOL("ShaderWaterVirtualSun", m_shaderWaterVirtualSun, 1,
			"Glints off a sun ahead of the camera at the map sun's height."),
		WATER_REAL("ShaderWaterRefraction", m_shaderWaterRefraction, 0.0f, 0.1f, 0.001f, 1,
			"How far the waves bend the seabed, as a fraction of the screen."),
		WATER_REAL("ShaderWaterWaveShading", m_shaderWaterWaveShading, 0.0f, 3.0f, 0.05f, 1,
			"How much the waves light and shade the water's own colour on shader model 3 cards. 0 leaves it flat."),
		WATER_REAL("ShaderWaterTexturePattern", m_shaderWaterTexturePattern, 0.0f, 1.0f, 0.01f, 1,
			"How much of the water texture's pattern shows over the water colour. 0 is the plain colour, 1 the full pattern."),
		WATER_REAL("ShaderWaterOpenReach", m_shaderWaterOpenReach, 1.0f, 2550.0f, 10.0f, 1,
			"World units from shore at which water counts as fully open. The enclosed look fades into the open look over this distance."),
		WATER_REAL("ShaderWaterStochasticSize", m_shaderWaterStochasticSize, 0.0f, 1000.0f, 1.0f, 1,
			"World units between the cells that shift the textures to hide tiling. 0 turns it off."),
		WATER_BOOL("ShaderWaterStochasticSeabed", m_shaderWaterStochasticSeabed, 1,
			"Hex cells also hide the tiling of the terrain under standing water."),
		WATER_REAL("ShaderWaterSwellScale", m_shaderWaterSwellScale, 1.0f, 3000.0f, 1.0f, 1,
			"World units one swell pattern covers. Higher gives longer swells."),
		WATER_REAL("ShaderWaterSwellSpeed", m_shaderWaterSwellSpeed, -200.0f, 200.0f, 1.0f, 1,
			"World units a second the swell drifts. 0 holds it still."),
		WATER_REAL("ShaderWaterPlanarStrength", m_shaderWaterPlanarStrength, 0.0f, 1.0f, 0.01f, 1,
			"Reflection the mirrored scene adds on top of the sky's."),
		WATER_REAL("ShaderWaterPlanarDistortion", m_shaderWaterPlanarDistortion, 0.0f, 0.1f, 0.001f, 1,
			"How far the waves bend the mirrored scene, as a fraction of the screen."),
		WATER_BOOL("ShaderWaterClearReflections", m_shaderWaterClearReflections, 1,
			"Shadows leave the sky and mirrored scene in the water as bright as around them."),
		WATER_BOOL("ShaderWaterSoftShadows", m_shaderWaterSoftShadows, 1,
			"Shadows in the water blur with depth and sway with the ripples."),
	};

	const int s_keyCount = sizeof(s_keys) / sizeof(s_keys[0]);

	void readKey(const WaterKey &k, const WaterTransparencySetting *wt, float v[3])
	{
		v[0] = v[1] = v[2] = 0.0f;
		switch (k.kind)
		{
			case WBQT_WATER_FLOAT:
				v[0] = wt->*(k.real);
				break;
			case WBQT_WATER_BOOL:
				v[0] = (wt->*(k.flag)) ? 1.0f : 0.0f;
				break;
			case WBQT_WATER_COLOR:
			{
				const RGBColor &c = wt->*(k.color);
				const Bool unset = c.red < 0.0f || c.green < 0.0f || c.blue < 0.0f;
				v[0] = unset ? -1.0f : c.red * 255.0f;
				v[1] = unset ? -1.0f : c.green * 255.0f;
				v[2] = unset ? -1.0f : c.blue * 255.0f;
				break;
			}
		}
	}

	RGBColor toColor(const float v[3])
	{
		RGBColor c;
		const Bool unset = v[0] < 0.0f || v[1] < 0.0f || v[2] < 0.0f;
		c.red = unset ? -1.0f : v[0] / 255.0f;
		c.green = unset ? -1.0f : v[1] / 255.0f;
		c.blue = unset ? -1.0f : v[2] / 255.0f;
		return c;
	}

	Bool keyDiffers(const WaterKey &k, const WaterTransparencySetting *wt, const float v[3])
	{
		switch (k.kind)
		{
			case WBQT_WATER_FLOAT:
				return wt->*(k.real) != v[0];
			case WBQT_WATER_BOOL:
				return (wt->*(k.flag) != FALSE) != (v[0] >= 0.5f);
			case WBQT_WATER_COLOR:
			{
				const RGBColor c = toColor(v);
				const RGBColor &now = wt->*(k.color);
				return now.red != c.red || now.green != c.green || now.blue != c.blue;
			}
		}
		return FALSE;
	}

	void writeKey(const WaterKey &k, WaterTransparencySetting *wt, const float v[3])
	{
		switch (k.kind)
		{
			case WBQT_WATER_FLOAT:
				wt->*(k.real) = v[0];
				break;
			case WBQT_WATER_BOOL:
				wt->*(k.flag) = v[0] >= 0.5f;
				break;
			case WBQT_WATER_COLOR:
				wt->*(k.color) = toColor(v);
				break;
		}
	}
}

extern "C" int WBQtWaterTuning_Count(void)
{
	return s_keyCount;
}

extern "C" int WBQtWaterTuning_GetDesc(int i, WBQtWaterTuningDesc *out)
{
	if (i < 0 || i >= s_keyCount || out == NULL)
	{
		return 0;
	}
	const WaterKey &k = s_keys[i];
	out->key = k.key;
	out->help = k.help;
	out->kind = k.kind;
	out->lo = k.lo;
	out->hi = k.hi;
	out->step = k.step;
	out->advanced = k.advanced;
	return 1;
}

extern "C" void WBQtWaterTuning_GetBase(int i, float v[3])
{
	if (v == NULL)
	{
		return;
	}
	v[0] = v[1] = v[2] = 0.0f;
	const WaterTransparencySetting *base = TheWaterTransparency.getNonOverloadedPointer();
	if (i < 0 || i >= s_keyCount || base == NULL)
	{
		return;
	}
	readKey(s_keys[i], base, v);
}

extern "C" void WBQtWaterTuning_SetLive(int i, const float v[3])
{
	if (i < 0 || i >= s_keyCount || v == NULL)
	{
		return;
	}
	const WaterKey &k = s_keys[i];

	// A value the water already has needs no override made for it.
	const WaterTransparencySetting *now = TheWaterTransparency;
	if (now == NULL || !keyDiffers(k, now, v))
	{
		return;
	}
	WaterTransparencySetting *wt = WBMapIni_EnsureWaterOverride();
	if (wt == NULL)
	{
		return;
	}
	writeKey(k, wt, v);

	WbView3d *view = CWorldBuilderDoc::GetActive3DView();
	if (view == NULL)
	{
		return;
	}
	if (k.rebuildsTerrain && TheTerrainRenderObject != NULL)
	{
		IRegion2D range = {0,0,0,0};
		view->updateHeightMapInView(TheTerrainRenderObject->getMap(), false, range);
	}
	view->Invalidate(false);
}

extern "C" void WBQtWaterTuning_NoteSaved(void)
{
	CWorldBuilderDoc *doc = CWorldBuilderDoc::GetActiveDoc();
	if (doc != NULL)
	{
		doc->noteMapIniSaved();
	}
}

#endif // RTS_HAS_QT
