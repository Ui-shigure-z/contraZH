// WBQtScorchBridge.h -- self-contained facade for the Qt Scorch Options panel.
//
// Like WBQtPanelBridge.h this header carries ONLY int/double (no Qt or MFC types), so the
// MFC ScorchOptions TU and the Qt panel TU can talk without including each other's headers.
// Kept as its own header (not folded into WBQtPanelBridge.h) so the shared facade stays
// untouched. Included by qt/panels/WBQtScorchPanel.cpp (Qt side) and
// src/WBQtScorchBridge.cpp (MFC side).
//
// The MFC ScorchOptions is still Create()d as the hidden OFF fallback and owns the selection
// statics (m_scorchtype / m_scorchsize). It edits the currently selected scorch MapObject's
// Dict (TheKey_scorchType / TheKey_objectRadius) via DictItemUndoable in changeScorch() /
// changeSize(); ScorchOptions::update() re-reads those statics from the single selected
// scorch on every selection change. This bridge lets the Qt panel read the same statics and
// drive the same Dict edits, so the ScorchTool path keeps working unchanged.
#ifndef WB_QT_SCORCH_BRIDGE_H
#define WB_QT_SCORCH_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

// --- Scorch panel: reverse, Qt widget -> MFC ScorchOptions (implemented MFC-side,
// WBQtScorchBridge.cpp -> ScorchOptions::qt*). No strings/pointers cross the seam.

// Scorch type combo. The entry list is fixed (the values baked into the IDD_SCORCH_OPTIONS
// combo in WorldBuilder.rc); GetTypeCount / GetTypeName expose it so the Qt combo mirrors it
// exactly. The selected index maps 1:1 to the Scorches enum value the MFC panel stores.
int  WBQtScorch_GetTypeCount(void);
int  WBQtScorch_GetTypeName(int index, char *nameOut, int cap);

// Current type (Scorches enum value == combo index) and size (world units / objectRadius).
// GetType/GetSize read m_scorchtype/m_scorchsize; SetType mirrors OnChangeScorchtype (stores
// the type and runs changeScorch()); SetSize mirrors OnChangeSizeEdit / the size popup-slider
// (stores the size and runs changeSize()). Both build a DictItemUndoable against every
// selected scorch, exactly like the MFC handlers.
int    WBQtScorch_GetType(void);
void   WBQtScorch_SetType(int type);
double WBQtScorch_GetSize(void);
void   WBQtScorch_SetSize(double size);

// Forward (Qt-side, WBQtScorchPanel.cpp): ScorchTool calls ScorchOptions::update() on
// activate / selection / placement; a guarded WBQtScorch_PushRefresh() re-seeds the Qt panel
// (type combo + size field) from the current statics so the two stay in step.
void WBQtScorch_PushRefresh(void);

#ifdef __cplusplus
}
#endif

#endif // WB_QT_SCORCH_BRIDGE_H
