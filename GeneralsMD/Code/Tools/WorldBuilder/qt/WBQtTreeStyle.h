// WBQtTreeStyle.h -- MFC-style branch connector lines for the migrated Qt trees.
//
// The MFC SysTreeView32 controls (TVS_HASLINES) draw elbow/connector lines linking each parent to
// its children. Qt's default Windows style (WindowsVista) draws modern triangle handles with NO
// connector lines, so a straight port looks "line-less" next to the MFC original. The Fusion style
// DOES draw the connector lines and honours the active (dark/light) palette, so we set Fusion on
// each tree individually -- scoped to that widget, leaving the rest of the app on its theme style.
//
// A single Fusion QStyle instance is created lazily and shared by every tree (setStyle does not
// take ownership, so the instance must outlive the widgets -- it lives for the app's lifetime).
#ifndef WB_QT_TREE_STYLE_H
#define WB_QT_TREE_STYLE_H

class QAbstractItemView;

namespace WBQtTreeStyle
{
	// Give `view` MFC-style branch connector lines (via a shared Fusion QStyle). Safe to call on
	// any tree view; a NULL view is ignored.
	void applyTreeLines(QAbstractItemView *view);
}

#endif // WB_QT_TREE_STYLE_H
