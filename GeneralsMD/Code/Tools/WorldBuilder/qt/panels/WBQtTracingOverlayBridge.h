// WBQtTracingOverlayBridge.h -- opaque facade for the Qt Tracing Overlay settings window.
//
// Like the other per-panel bridge headers this carries ONLY plain C types, so the MFC
// side (wbview3d.cpp) can open/close the Qt window and the Qt side can read/write the
// persisted overlay settings without either side including the other's headers.
#ifndef WB_QT_TRACING_OVERLAY_BRIDGE_H
#define WB_QT_TRACING_OVERLAY_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

// --- Qt side (qt/panels/WBQtTracingOverlayWindow.cpp) -----------------------------------
// Open (or raise) the modeless settings window. Returns 0 when Qt is not up yet
// (qApp == NULL) so the caller can fall back to the MFC dialog.
int  WBQtTracingOverlay_Open(void *frameHwnd);
// Hide the window if it is open (overlay turned off). Persists the current values first.
void WBQtTracingOverlay_Close(void);

// --- MFC side (src/WBQtTracingOverlayBridge.cpp) ----------------------------------------
// The persisted [Appearance] settings (TracingOverlayOptions statics).
int  WBQtTracingOverlayData_GetOpacityPct(void);	// 0..100
int  WBQtTracingOverlayData_GetFilter(void);		// 0 = Default (linear), 1 = Nearest
// Push UI values into the statics + DrawObject (live preview). persist != 0 also writes
// the [Appearance] profile keys -- pass 0 while the slider drags, 1 on commit points.
void WBQtTracingOverlayData_SetFromUi(int opacityPct, int filter, int persist);

#ifdef __cplusplus
}
#endif

#endif // WB_QT_TRACING_OVERLAY_BRIDGE_H
