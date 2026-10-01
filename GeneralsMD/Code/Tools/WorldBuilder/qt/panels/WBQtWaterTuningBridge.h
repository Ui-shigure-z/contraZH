// WBQtWaterTuningBridge.h -- opaque facade for the Qt Water tuning window (File > Map.ini >
// Water tuning).
//
// The MFC side owns the key table and writes the map's live WaterTransparency override, so the
// 3D view shows each change. The Qt side owns the rows and the WaterTransparency block of the
// map's map.ini.
#ifndef WB_QT_WATER_TUNING_BRIDGE_H
#define WB_QT_WATER_TUNING_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

enum
{
	WBQT_WATER_FLOAT = 0,
	WBQT_WATER_BOOL = 1,
	WBQT_WATER_COLOR = 2
};

typedef struct WBQtWaterTuningDesc
{
	const char *key;	// INI key in the WaterTransparency block
	const char *help;	// tooltip
	int kind;			// WBQT_WATER_*
	float lo;
	float hi;
	float step;
	int advanced;		// 1 hides the row until "Advanced keys" is ticked
} WBQtWaterTuningDesc;

// ====== Qt -> MFC (implemented in src/WBQtWaterTuningBridge.cpp) ======

int  WBQtWaterTuning_Count(void);
int  WBQtWaterTuning_GetDesc(int i, WBQtWaterTuningDesc *out);

// A float or bool key uses v[0]. A colour uses v[0..2] as 0 to 255, negative when unset.
// GetBase reads the Water.ini value under any map override.
void WBQtWaterTuning_GetBase(int i, float v[3]);
void WBQtWaterTuning_SetLive(int i, const float v[3]);

// The window saved map.ini itself, so the auto-reload watch must not treat it as an outside edit.
void WBQtWaterTuning_NoteSaved(void);

// ====== MFC -> Qt (implemented in qt/panels/WBQtWaterTuningPanel.cpp) ======

void WBQtWaterTuning_Open(void *frameHwnd, const char *iniPath);

// Another map is loading: save what is pending and hide.
void WBQtWaterTuning_MapChanged(void);

// The map.ini overrides were reloaded or dropped: read the file again while visible.
void WBQtWaterTuning_PushRefresh(void);

#ifdef __cplusplus
}
#endif

#endif // WB_QT_WATER_TUNING_BRIDGE_H
