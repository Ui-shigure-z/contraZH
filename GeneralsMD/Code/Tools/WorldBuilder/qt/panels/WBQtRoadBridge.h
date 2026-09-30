// WBQtRoadBridge.h -- self-contained facade for the Qt Road Options panel.
//
// Like WBQtPanelBridge.h this header carries ONLY int/double/char* (no Qt or MFC types), so
// the MFC RoadOptions TU and the Qt panel TU can talk without including each other's headers.
// Kept as its own header (not folded into WBQtPanelBridge.h) so the shared facade stays
// untouched. Included by qt/panels/WBQtRoadPanel.cpp (Qt side) and src/WBQtRoadBridge.cpp
// (MFC side).
//
// The MFC RoadOptions is still Create()d as the hidden OFF fallback and owns the road-type
// selection statics (m_currentRoadIndex / m_currentRoadName) plus the corner-type / join
// state (m_angleCorners / m_tightCurve / m_doJoin) that RoadTool + the apply path read. This
// bridge lets the Qt Road panel MIRROR the road/bridge list (from TheTerrainRoads) by index,
// drive those statics, and fire the same command handlers (applyToSelection / OnJoin /
// ChangeRoadType), so RoadTool + PointerTool keep working unchanged. PointerTool / RoadTool
// call RoadOptions::updateSelection() when a road selection changes; a guarded
// WBQtRoad_PushRefresh() re-seeds the Qt panel from the current selection so the two stay in
// step. This bridge reaches the model through RoadOptions::qt* statics (defined MFC-side) that
// reuse the same getters / handlers the MFC message map uses, so the tools keep working.
#ifndef WB_QT_ROAD_BRIDGE_H
#define WB_QT_ROAD_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

// Corner type of a road segment (mirrors the three mutually-exclusive MFC radio-style buttons
// IDC_BROAD_CURVE / IDC_TIGHT_CURVE / IDC_ANGLED):
#define WBQT_ROAD_CORNER_BROAD   0
#define WBQT_ROAD_CORNER_TIGHT   1
#define WBQT_ROAD_CORNER_ANGLED  2

// --- Road panel: reverse, Qt widget -> MFC RoadOptions (implemented MFC-side, --------------
//     src/WBQtRoadBridge.cpp -> RoadOptions::qt*). Strings copy into caller buffers (no
//     ownership crosses the seam).

// Road / bridge list (from TheTerrainRoads): enumerate to build the tree. The index is the
// running position in the (roads-then-bridges) enumeration order -- exactly the lParam the MFC
// tree stores, so it round-trips through SelectIndex. GetEntry fills the group node ("Roads" or
// "Bridges") + the leaf (display) name; GetFullName gives the same name for the search filter.
int  WBQtRoad_GetCount(void);
int  WBQtRoad_GetEntry(int index, char *groupOut, char *leafOut, int cap);
int  WBQtRoad_GetFullName(int index, char *nameOut, int cap);

// Selection (Qt -> MFC): make index the current road type (drives ChangeRoadType via Apply and
// the RoadTool placement path). Mirrors the MFC TVN_SELCHANGED leaf branch (sets
// m_currentRoadIndex + m_currentRoadName). GetSelectedIndex / GetCurrentName read them back.
void WBQtRoad_SelectIndex(int index);
int  WBQtRoad_GetSelectedIndex(void);
int  WBQtRoad_GetCurrentName(char *nameOut, int cap);

// Corner type + join. Get* read the current sticky state (m_angleCorners / m_tightCurve /
// m_doJoin); Set* mirror OnBroadCurve / OnTightCurve / OnAngled (each sets the sticky flags and
// runs applyToSelection) and OnJoin (sets m_doJoin and runs the join undoable). Setting a corner
// type applies it to the current road selection.
int  WBQtRoad_GetCornerType(void);
void WBQtRoad_SetCornerType(int cornerType);
int  WBQtRoad_GetJoin(void);
void WBQtRoad_SetJoin(int on);

// Apply the currently-selected road type to the selected road segments (mirrors OnApplyRoad ->
// ChangeRoadType(m_currentRoadName)).
void WBQtRoad_ApplyRoadType(void);

// Road snap distance (world units), clamped to [0.2, 5.0] and persisted in the registry under
// the same section/key the MFC panel uses ("RoadOptionPanel" / "RoadSnappingDistance"). Mirrors
// OnEditSnapPoint. Get returns the stored value (default 1.0).
double WBQtRoad_GetSnapDistance(void);
void   WBQtRoad_SetSnapDistance(double distance);

// Selection-derived checkbox state, mirroring RoadOptions::updateSelection(): fills the corner
// type (WBQT_ROAD_CORNER_*) and join flag that reflect the current road selection (falling back
// to the sticky state when nothing is selected). *mixedOut is non-zero when the selection is
// mixed (multiple corner types) so the panel can clear its checkboxes like the MFC path. Fills
// the road name of the single selected road (empty if none / multiple names).
void WBQtRoad_GetSelectionState(int *cornerTypeOut, int *joinOut, int *mixedOut,
	char *roadNameOut, int cap);

// Forward (Qt-side, WBQtRoadPanel.cpp): RoadTool / PointerTool call RoadOptions::updateSelection()
// when a road selection changes; a guarded WBQtRoad_PushRefresh() re-seeds the Qt panel from the
// current selection so the two stay in step.
void WBQtRoad_PushRefresh(void);

#ifdef __cplusplus
}
#endif

#endif // WB_QT_ROAD_BRIDGE_H
