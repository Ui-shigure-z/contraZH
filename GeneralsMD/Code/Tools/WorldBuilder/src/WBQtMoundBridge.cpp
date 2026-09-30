// WBQtMoundBridge.cpp -- the MFC side of the Qt Mound-panel seam. See WBQtBrushBridge.cpp
// for the pattern. Plain MFC TU (no Qt include); reverse callbacks -> MoundTool statics,
// resolved against the exe at the final link. Whole body guarded by RTS_HAS_QT so the OFF
// build compiles it to an empty object. (MoundTool spells height "moundHeight".)
#include "StdAfx.h"
#include "MoundTool.h"
#include "qt/WBQtPanelBridge.h"

#ifdef RTS_HAS_QT
extern "C" {

void WBQtMound_SetWidth(int v)        { MoundTool::setWidth(v); }
void WBQtMound_SetFeather(int v)      { MoundTool::setFeather(v); }
void WBQtMound_SetHeight(int v)       { MoundTool::setMoundHeight(v); }
void WBQtMound_ToggleMirror(void)     { MoundTool::toggleMirror(); }
void WBQtMound_ToggleMirrorX(void)    { MoundTool::toggleMirrorX(); }
void WBQtMound_ToggleMirrorY(void)    { MoundTool::toggleMirrorY(); }
void WBQtMound_ToggleMirrorXY(void)   { MoundTool::toggleMirrorXY(); }

int WBQtMound_GetWidth(void)    { return MoundTool::getWidth(); }
int WBQtMound_GetFeather(void)  { return MoundTool::getFeather(); }
int WBQtMound_GetHeight(void)   { return MoundTool::getMoundHeight(); }
int WBQtMound_GetMirror(void)   { return MoundTool::getEnableMirror() ? 1 : 0; }
int WBQtMound_GetMirrorX(void)  { return MoundTool::getMirrorX() ? 1 : 0; }
int WBQtMound_GetMirrorY(void)  { return MoundTool::getMirrorY() ? 1 : 0; }
int WBQtMound_GetMirrorXY(void) { return MoundTool::getMirrorXY() ? 1 : 0; }

}
#endif
