// WBQtMapIniCodeEditor.h -- the map.ini code view shared by the map.ini editor and the
// loader's report: a plain-text editor with a line-number gutter and, when enabled, fold
// arrows that collapse a top-level block to its header line.
#ifndef WB_QT_MAPINI_CODE_EDITOR_H
#define WB_QT_MAPINI_CODE_EDITOR_H

#include <QColor>
#include <QList>
#include <QPair>
#include <QPlainTextEdit>
#include <QString>
#include <QVector>

// Badge text and colour for a WBQT_MAPINI_BLOCK_* status, shared by the report and the editor.
QString WBQtMapIniStatusText(int status);
QColor WBQtMapIniStatusColour(int status);

class WBQtMapIniCodeEditor : public QPlainTextEdit
{
public:
	explicit WBQtMapIniCodeEditor(QWidget *parent = 0);

	// The number shown beside the first line; a block excerpt shows its map.ini line numbers.
	void setFirstLineNumber(int number);

	// Folding adds the arrow column to the gutter.
	void setFoldingEnabled(bool enabled);
	// The foldable blocks as (header line, last line), 0-based. Folds whose header survives stay
	// folded; anything else hidden is shown again.
	void setFoldRanges(const QList<QPair<int, int> > &ranges);
	bool isFolded(int headerLine) const;
	void setFolded(int headerLine, bool folded);
	void setAllFolded(bool folded);
	// Unfold whatever hides this line.
	void revealLine(int line);
	// Put the cursor on this line and scroll it to the top of the view.
	void scrollLineToTop(int line);

	int gutterWidth() const;
	void paintGutter(QPaintEvent *event);
	void gutterPressed(const QPoint &pos);

protected:
	virtual void resizeEvent(QResizeEvent *event);
	virtual void paintEvent(QPaintEvent *event);

private:
	class Gutter;

	int foldRangeAt(int headerLine) const;		///< index into m_foldRanges, or -1
	int numberWidth() const;
	void updateGutterGeometry();
	// Show or hide each line by index (missing entries show); one relayout for the lot.
	void applyVisibility(const QVector<bool> &visible);

	Gutter *m_gutter;
	int m_firstLine;
	bool m_folding;
	QList<QPair<int, int> > m_foldRanges;
};

#endif // WB_QT_MAPINI_CODE_EDITOR_H
