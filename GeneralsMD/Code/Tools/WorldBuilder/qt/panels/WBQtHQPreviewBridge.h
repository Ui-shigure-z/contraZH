// WBQtHQPreviewBridge.h -- opaque facade for the Qt HQ map preview dialog (File > Map.ini >
// Generate HQ tga).
//
// The MFC side holds the captured render and shades it. The Qt side shows the result and the
// shading controls, and asks for a new image whenever a control changes.
#ifndef WB_QT_HQ_PREVIEW_BRIDGE_H
#define WB_QT_HQ_PREVIEW_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

// Colours are RGB, 0 to 255.
typedef struct WBQtHQPreviewParams
{
	float relief;
	float elevation;
	float waterFalloff;
	int shallow[3];
	int deep[3];
} WBQtHQPreviewParams;

// ====== Qt -> MFC (implemented in src/WBQtHQPreviewBridge.cpp) ======

int  WBQtHQPreview_Size(void);
void WBQtHQPreview_GetDefaults(WBQtHQPreviewParams *out);
// The settings last saved with, or the defaults.
void WBQtHQPreview_GetLast(WBQtHQPreviewParams *out);
// Fills Size()^2 BGRA pixels, top row north.
void WBQtHQPreview_Compose(const WBQtHQPreviewParams *params, unsigned char *bgra);
// Writes the tga and remembers the settings. Returns 1 on success.
int  WBQtHQPreview_Save(const WBQtHQPreviewParams *params);

// ====== MFC only (implemented in src/WBQtHQPreviewBridge.cpp) ======

// Renders the map and runs the dialog. Returns 1 when saved, 0 when cancelled, -1 on failure.
int  WBQtHQPreview_Run(void *view, const char *mapPath);

// ====== MFC -> Qt (implemented in qt/panels/WBQtHQPreviewDialog.cpp) ======

// Modal. Returns 1 when the user saved.
int  WBQtHQPreview_Show(const char *tgaPath);

#ifdef __cplusplus
}
#endif

#endif // WB_QT_HQ_PREVIEW_BRIDGE_H
