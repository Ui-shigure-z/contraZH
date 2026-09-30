// WBQtTerrainModalBridge.h -- opaque facade for the Qt "missing terrain texture" picker
// (Tier 5b), the native rebuild of TerrainModal. It pops per unresolved texture class
// during map load (validateTexClasses) and for Texture Sizing > Remap -- the load path
// can run before WBQt_Startup, so _Run returns -1 when Qt is not up and the callers fall
// back to the MFC dialog. The swatch preview reuses the WBQtTerrainMaterial bridge.
#ifndef WB_QT_TERRAINMODAL_BRIDGE_H
#define WB_QT_TERRAINMODAL_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

// ====== MFC -> Qt (implemented in qt/panels/WBQtTerrainModalDialog.cpp) ======

// Run the modal picker for one missing texture class. heightMapEdit = the
// WorldHeightMapEdit* being validated (drives the used-class initial-selection bump).
// pickedOut = the chosen global texture class. Returns 1 OK, 0 cancel, -1 Qt unavailable.
int WBQtTerrainModal_Run(void *frameHwnd, const char *missingPath, void *heightMapEdit,
	int *pickedOut);

// ====== Qt -> MFC (implemented in src/WBQtTerrainModalBridge.cpp) ======

// Build the rows (== TerrainModal::updateTextures/addTerrain): every global texture
// class, grouped by its INI TerrainClass name -- or under **LegacyGDF/<path> for legacy
// entries. Returns the row count.
int  WBQtTerrainModalData_Build(void *heightMapEdit);
void WBQtTerrainModalData_GetInfo(int i, char *groupPathOut, int groupCap,
	char *leafOut, int leafCap, int *texClassOut);
int  WBQtTerrainModalData_GetInitialSelection(void);

// The selected texture's UI-name leaf (== TerrainModal::updateLabel).
void WBQtTerrainModalData_GetUiNameLeaf(int texClass, char *bufOut, int cap);

// Selection side effect kept from MFC: the dialog drives the shared foreground texture
// class (which is also what the swatch preview renders).
void WBQtTerrainModal_SetFgTexClass(int texClass);

#ifdef __cplusplus
}
#endif

#endif // WB_QT_TERRAINMODAL_BRIDGE_H
