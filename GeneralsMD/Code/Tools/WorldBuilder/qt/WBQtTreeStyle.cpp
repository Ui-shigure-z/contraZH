// WBQtTreeStyle.cpp -- see WBQtTreeStyle.h.
#include "WBQtTreeStyle.h"

#include <QAbstractItemView>
#include <QPainter>
#include <QProxyStyle>
#include <QScrollBar>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleOption>
#include <QtGlobal>

namespace
{
	// A proxy over Fusion that draws the tree's branch CONNECTOR LINES with a palette-derived pen,
	// so they stay visible in both dark and light themes. Fusion draws those lines with a fixed
	// dark pen (near-black) that vanishes on a dark window; we let the base style paint the
	// primitive (which keeps the expand/collapse arrows exactly as Fusion draws them) then overpaint
	// just the elbow/vertical/horizontal line segments with a colour computed from the active
	// palette, so a System/Dark/Light switch re-colours them automatically on the next repaint.
	class WBQtTreeLineStyle : public QProxyStyle
	{
	public:
		explicit WBQtTreeLineStyle(QStyle *base) : QProxyStyle(base) {}

		virtual void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
			QPainter *painter, const QWidget *widget) const
		{
			if (element != PE_IndicatorBranch || option == NULL)
			{
				QProxyStyle::drawPrimitive(element, option, painter, widget);
				return;
			}

			// We draw the connector lines ourselves so we control their colour. The base style
			// (QCommonStyle, via Fusion) draws BOTH the lines and the expand/collapse arrow in this
			// one primitive with a fixed dark pen; letting it run and overpainting produced a ghost
			// double-line. Instead we let the base draw ONLY the arrow -- clipped to a small centred
			// box so its own line-drawing is masked out -- then draw every line with our pen.
			const QRect rect = option->rect;
			const int cx = rect.center().x();
			const int cy = rect.center().y();
			const State s = option->state;

			// Arrow only: clip the base draw to a centred square around the handle so none of its
			// full-cell connector lines survive the clip.
			const int half = qMin(rect.width(), rect.height()) / 3;
			if (s & State_Children)
			{
				QRect arrowClip(cx - half, cy - half, half * 2, half * 2);
				painter->save();
				painter->setClipRect(arrowClip);
				QProxyStyle::drawPrimitive(element, option, painter, widget);
				painter->restore();
			}

			// No connector lines in the ROOT column: a full-height line hugging the panel edge
			// (with a stub into every top-level item) reads as clutter, and it overdrew the
			// expand arrows so parents did not look expandable. Root items show just their
			// arrow; lines only connect the members of an expanded group. The root column is
			// the cell at logical x 0 -- rect is in viewport coordinates, so add back the
			// horizontal scroll offset before testing.
			int logicalLeft = rect.left();
			const QAbstractScrollArea *area = qobject_cast<const QAbstractScrollArea *>(widget);
			if (area != NULL && area->horizontalScrollBar() != NULL)
			{
				logicalLeft += area->horizontalScrollBar()->value();
			}
			if (logicalLeft <= 0)
			{
				return;
			}

			// A mid-grey between window and text reads well on both dark and light palettes.
			QColor lineColor = option->palette.color(QPalette::Mid);

			// Lines stop short of the expand arrow (gap = the arrow clip box) so they never
			// cross through it.
			const int gap = (s & State_Children) ? half : 0;

			painter->save();
			painter->setPen(lineColor);

			// Vertical run for items that have siblings continuing below them.
			if (s & State_Sibling)
			{
				painter->drawLine(cx, rect.top(), cx, cy - gap);
				painter->drawLine(cx, cy + gap, cx, rect.bottom());
			}
			// The last child in a group: the vertical run stops at this item's centre.
			else if (s & State_Item)
			{
				painter->drawLine(cx, rect.top(), cx, cy - gap);
			}
			// Horizontal elbow into the item itself.
			if (s & State_Item)
			{
				painter->drawLine(cx + gap, cy, rect.right(), cy);
			}
			// A collapsed/expanded parent has a short vertical stub above its handle so the line
			// meets its own parent's run.
			if (s & State_Children)
			{
				painter->drawLine(cx, rect.top(), cx, cy - gap);
			}

			painter->restore();
		}
	};

	// The one shared proxy style. Created on first use and never deleted (it must outlive every
	// tree it is set on; setStyle does not take ownership). QApplication owns the app lifetime, so
	// leaking this single object at exit is intentional and harmless. The proxy owns its Fusion base.
	QStyle *s_treeStyle = NULL;

	QStyle *treeStyle()
	{
		if (s_treeStyle == NULL)
		{
			QStyle *fusion = QStyleFactory::create("Fusion");
			if (fusion != NULL)
			{
				s_treeStyle = new WBQtTreeLineStyle(fusion);
			}
		}
		return s_treeStyle;
	}
}

namespace WBQtTreeStyle
{
	void applyTreeLines(QAbstractItemView *view)
	{
		if (view == NULL)
		{
			return;
		}
		QStyle *style = treeStyle();
		if (style != NULL)
		{
			view->setStyle(style);
		}
	}
}
