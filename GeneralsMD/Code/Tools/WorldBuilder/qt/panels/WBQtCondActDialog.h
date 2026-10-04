// WBQtCondActDialog.h -- the native Qt condition/action editor, replacing the MFC EditCondition /
// EditAction modals; one class serves both through the isAction flag. Run via WBQtCondAct_Run().
#ifndef WB_QT_CONDACT_DIALOG_H
#define WB_QT_CONDACT_DIALOG_H

#include <QDialog>
#include <QHash>
#include <QStringList>

class QLayout;
class QTreeWidgetItem;

namespace Ui { class WBQtCondActDialog; }	// generated from WBQtCondActDialog.ui

// Left: the template tree with a live filter, Favorites and Recent on top. Right: the item's
// sentence with each parameter as a clickable chip, its warnings, and the developer notes.
class WBQtCondActDialog : public QDialog
{
	Q_OBJECT
public:
	WBQtCondActDialog(void *item, bool isAction, QWidget *parent = 0);
	virtual ~WBQtCondActDialog();

	// Records the chosen type under Recent.
	virtual void accept();

protected:
	bool eventFilter(QObject *watched, QEvent *event);

private slots:
	void onFilterChanged(const QString &text);
	void onCurrentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
	void onChipClicked();
	void onTreeContextMenu(const QPoint &pos);
	void onNotesToggled(bool open);

private:
	void buildTree(const QString &filter);
	// A Favorites or Recent folder at the top, listing the saved paths that pass the filter.
	void addSavedFolder(const QString &title, const QStringList &paths, const QString &filter);
	void selectCurrentType(QTreeWidgetItem *leaf);
	void selectFirstMatch();
	void renderSentence();
	void updateWarnings();
	void showHelpForType(int type);
	void applyTreeFont();
	QStringList savedList(bool favorites) const;
	void setSavedList(bool favorites, const QStringList &paths);

	Ui::WBQtCondActDialog *m_ui;	// owns the static widget tree (WBQtCondActDialog.ui)

	void *m_item;
	int m_isAction;
	bool m_updating;
	QLayout *m_flow;				///< the sentence's words and chips
	QHash<QString, int> m_pathIndex;	///< template path -> template index
};

#endif // WB_QT_CONDACT_DIALOG_H
