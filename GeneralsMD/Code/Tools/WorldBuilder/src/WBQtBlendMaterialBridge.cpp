// WBQtBlendMaterialBridge.cpp -- the MFC side of the Qt Blend Material-panel seam.
//
// Plain MFC TU (no Qt include). Implements the reverse callbacks the Qt Blend Material panel
// (qt/panels/WBQtBlendMaterialPanel.cpp) fires when its controls change, plus the getters the
// panel reads to seed itself. The Qt static lib resolves these against the exe at the final
// link (extern "C" keeps the names stable).
//
// The three "tile gap" checkboxes drive BlendMaterial's statics (m_hvgap / m_dgap /
// m_revalblends) via the guarded BlendMaterial::qtSet* helpers, which also refresh the blend
// pointer tooltip -- exactly what the MFC On* handlers do. The four "mirror" checkboxes toggle
// AutoEdgeOutTool's mirror statics, the same way BlendMaterial::OnToggleMirror* does.
//
// Whole body is guarded by RTS_HAS_QT, so the default (Qt-off) build compiles this to an empty
// object and the MFC build is unchanged.
#include "StdAfx.h"
#include "resource.h"
#include "Lib/BaseType.h"
#include "BlendMaterial.h"
#include "AutoEdgeOutTool.h"
#include "qt/panels/WBQtBlendMaterialBridge.h"

#ifdef RTS_HAS_QT

//----------------------------------------------------------------------------------------
// BlendMaterial Qt-support statics (declared in BlendMaterial.h; defined here so they can
// reach the protected gap statics without churning BlendMaterial.cpp). Each setter also
// refreshes the blend pointer tooltip, matching the MFC On* handlers.
//----------------------------------------------------------------------------------------
void BlendMaterial::qtSetHorizVertGap(Bool on)
{
	m_hvgap = on;
	updateBlendPointerToolTip();
}

void BlendMaterial::qtSetDiagGap(Bool on)
{
	m_dgap = on;
	updateBlendPointerToolTip();
}

void BlendMaterial::qtSetRevalBlends(Bool on)
{
	m_revalblends = on;
	updateBlendPointerToolTip();
}

extern "C" {

//----------------------------------------------------------------------------------------
// Tile gap checkboxes. Getters reuse BlendMaterial's existing public accessors.
//----------------------------------------------------------------------------------------
void WBQtBlendMaterial_SetHorizVertGap(int on)
{
	BlendMaterial::qtSetHorizVertGap(on ? true : false);
}
int WBQtBlendMaterial_GetHorizVertGap(void)
{
	return BlendMaterial::isHorizVertGap() ? 1 : 0;
}
void WBQtBlendMaterial_SetDiagGap(int on)
{
	BlendMaterial::qtSetDiagGap(on ? true : false);
}
int WBQtBlendMaterial_GetDiagGap(void)
{
	return BlendMaterial::isDiagGap() ? 1 : 0;
}
void WBQtBlendMaterial_SetRevalBlends(int on)
{
	BlendMaterial::qtSetRevalBlends(on ? true : false);
}
int WBQtBlendMaterial_GetRevalBlends(void)
{
	return BlendMaterial::isRevalBlends() ? 1 : 0;
}

//----------------------------------------------------------------------------------------
// Mirror checkboxes. Toggles match BlendMaterial::OnToggleMirror*; getters read the (guarded)
// AutoEdgeOutTool accessors so the panel can seed the checkbox state.
//----------------------------------------------------------------------------------------
void WBQtBlendMaterial_ToggleMirror(void)   { AutoEdgeOutTool::toggleMirror(); }
void WBQtBlendMaterial_ToggleMirrorX(void)  { AutoEdgeOutTool::toggleMirrorX(); }
void WBQtBlendMaterial_ToggleMirrorY(void)  { AutoEdgeOutTool::toggleMirrorY(); }
void WBQtBlendMaterial_ToggleMirrorXY(void) { AutoEdgeOutTool::toggleMirrorXY(); }

int WBQtBlendMaterial_GetMirror(void)   { return AutoEdgeOutTool::getEnableMirror() ? 1 : 0; }
int WBQtBlendMaterial_GetMirrorX(void)  { return AutoEdgeOutTool::getMirrorX() ? 1 : 0; }
int WBQtBlendMaterial_GetMirrorY(void)  { return AutoEdgeOutTool::getMirrorY() ? 1 : 0; }
int WBQtBlendMaterial_GetMirrorXY(void) { return AutoEdgeOutTool::getMirrorXY() ? 1 : 0; }

}
#endif
