// WBQtBlendMaterialBridge.h -- self-contained opaque facade for the Qt Blend Material panel.
//
// Mirrors the style of the shared WBQtPanelBridge.h but is kept in its own header so this
// migration does not have to touch that shared file. Carries ONLY int/char* (no Qt or MFC
// types), so the MFC bridge TU and the Qt panel TU can share these declarations without
// including each other's headers. Both src/WBQtBlendMaterialBridge.cpp (MFC side, defines the
// reverse callbacks) and qt/panels/WBQtBlendMaterialPanel.cpp (Qt side, calls them) include it.
//
// The MFC BlendMaterial dialog stays created as the hidden OFF fallback and owns the three
// "gap" statics (m_hvgap / m_dgap / m_revalblends) that AutoEdgeOutTool::applyEdgeAt reads;
// the "mirror" statics live on AutoEdgeOutTool itself. The Qt panel reads that state to seed
// its checkboxes and drives the same setters/toggles, so the tool keeps working unchanged.
#ifndef WB_QT_BLEND_MATERIAL_BRIDGE_H
#define WB_QT_BLEND_MATERIAL_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

// --- Blend Material panel: reverse, Qt widget -> MFC/tool (implemented MFC-side, --------
//     src/WBQtBlendMaterialBridge.cpp). No forward push: like the MFC BlendMaterial, the
//     panel only reads state at seed time and when the user changes a control.

// The three "tile gap" checkboxes drive BlendMaterial's statics (via BlendMaterial::qtSet*),
// which also refresh the blend pointer tooltip, exactly like the MFC On* handlers.
void WBQtBlendMaterial_SetHorizVertGap(int on);
int  WBQtBlendMaterial_GetHorizVertGap(void);
void WBQtBlendMaterial_SetDiagGap(int on);
int  WBQtBlendMaterial_GetDiagGap(void);
void WBQtBlendMaterial_SetRevalBlends(int on);
int  WBQtBlendMaterial_GetRevalBlends(void);

// The four "mirror" checkboxes toggle AutoEdgeOutTool's mirror statics (same tool the Feather
// panel drives). Toggles match the MFC BlendMaterial::OnToggleMirror* handlers exactly.
void WBQtBlendMaterial_ToggleMirror(void);
void WBQtBlendMaterial_ToggleMirrorX(void);
void WBQtBlendMaterial_ToggleMirrorY(void);
void WBQtBlendMaterial_ToggleMirrorXY(void);
int  WBQtBlendMaterial_GetMirror(void);
int  WBQtBlendMaterial_GetMirrorX(void);
int  WBQtBlendMaterial_GetMirrorY(void);
int  WBQtBlendMaterial_GetMirrorXY(void);

#ifdef __cplusplus
}
#endif

#endif // WB_QT_BLEND_MATERIAL_BRIDGE_H
