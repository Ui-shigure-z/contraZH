// WBQtCondActDialog.cpp -- see WBQtCondActDialog.h.
#include "WBQtCondActDialog.h"
#include "ui_WBQtCondActDialog.h"
#include "WBQtCondActBridge.h"
#include "WBQtTreeStyle.h"
#include "WBQtCondActDialogClassic.h"

// The script editor's "New design" setting (WBQtScriptBridge.cpp).
extern "C" int WBQtScript_GetNewDesign(void);

#include <QApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QToolButton>
#include <QTreeWidget>

namespace
{
	const int kNameCap = 512;
	const int kTextCap = 1024;
	const int kBigCap = 4096;
	const int kRecentMax = 10;
	const int kSavedListCap = 65536;

	// Item data role holding the template index on leaves (-1 on folders).
	const int kTemplateRole = Qt::UserRole;

	QString templateName(int isAction, int i)
	{
		char buf[kNameCap];
		buf[0] = 0;
		WBQtCondActData_GetTemplateName(isAction, i, buf, sizeof(buf));
		return QString::fromLocal8Bit(buf);
	}

	QString templateName2(int isAction, int i)
	{
		char buf[kNameCap];
		buf[0] = 0;
		WBQtCondActData_GetTemplateName2(isAction, i, buf, sizeof(buf));
		return QString::fromLocal8Bit(buf);
	}

	QString templateHelp(int isAction, int i)
	{
		char buf[kBigCap];
		buf[0] = 0;
		WBQtCondActData_GetTemplateHelp(isAction, i, buf, sizeof(buf));
		// == ParseHelpText: the help strings carry literal "\n" escapes.
		return QString::fromLocal8Bit(buf).replace("\\n", "\n");
	}

	QColor familyColour(int family)
	{
		switch (family)
		{
			case WBQT_PARAM_THING:	return QColor(55, 105, 165);
			case WBQT_PARAM_PLAYER:	return QColor(160, 100, 40);
			case WBQT_PARAM_PLACE:	return QColor(55, 130, 75);
			case WBQT_PARAM_NUMBER:	return QColor(70, 110, 120);
			case WBQT_PARAM_TEXT:	return QColor(125, 85, 165);
			case WBQT_PARAM_LOGIC:	return QColor(140, 115, 35);
			default:				return QColor(95, 95, 95);
		}
	}

	// Lays its widgets out left to right and wraps, centring each line's items vertically.
	class FlowLayout : public QLayout
	{
	public:
		FlowLayout(QWidget *parent, int spacing) : QLayout(parent), m_spacing(spacing)
		{
			setContentsMargins(8, 8, 8, 8);
		}

		virtual ~FlowLayout()
		{
			QLayoutItem *item;
			while ((item = takeAt(0)) != NULL)
			{
				delete item;
			}
		}

		virtual void addItem(QLayoutItem *item) { m_items.append(item); }
		virtual int count() const { return m_items.size(); }
		virtual QLayoutItem *itemAt(int index) const { return m_items.value(index); }
		virtual QLayoutItem *takeAt(int index)
		{
			return (index >= 0 && index < m_items.size()) ? m_items.takeAt(index) : NULL;
		}
		virtual Qt::Orientations expandingDirections() const { return 0; }
		virtual bool hasHeightForWidth() const { return true; }
		virtual int heightForWidth(int width) const { return doLayout(QRect(0, 0, width, 0), true); }
		virtual void setGeometry(const QRect &rect)
		{
			QLayout::setGeometry(rect);
			doLayout(rect, false);
		}
		virtual QSize sizeHint() const { return minimumSize(); }
		virtual QSize minimumSize() const
		{
			QSize size;
			for (int i = 0; i < m_items.size(); ++i)
			{
				size = size.expandedTo(m_items.at(i)->minimumSize());
			}
			int left, top, right, bottom;
			getContentsMargins(&left, &top, &right, &bottom);
			return size + QSize(left + right, top + bottom);
		}

	private:
		int doLayout(const QRect &rect, bool testOnly) const
		{
			int left, top, right, bottom;
			getContentsMargins(&left, &top, &right, &bottom);
			const QRect area = rect.adjusted(left, top, -right, -bottom);
			int x = area.x();
			int y = area.y();
			int lineHeight = 0;
			int lineStart = 0;
			for (int i = 0; i <= m_items.size(); ++i)
			{
				const bool last = (i == m_items.size());
				const QSize hint = last ? QSize() : m_items.at(i)->sizeHint();
				const bool wrap = !last && x > area.x() && x + hint.width() > area.right() + 1;
				if (last || wrap)
				{
					// Place the finished line, centred on its tallest item.
					if (!testOnly)
					{
						int lx = area.x();
						for (int j = lineStart; j < i; ++j)
						{
							const QSize h = m_items.at(j)->sizeHint();
							m_items.at(j)->setGeometry(QRect(QPoint(lx, y + (lineHeight - h.height()) / 2), h));
							lx += h.width() + m_spacing;
						}
					}
					if (last)
					{
						break;
					}
					x = area.x();
					y += lineHeight + m_spacing;
					lineHeight = 0;
					lineStart = i;
				}
				x += hint.width() + m_spacing;
				lineHeight = qMax(lineHeight, hint.height());
			}
			return y + lineHeight - rect.y() + bottom;
		}

		QList<QLayoutItem *> m_items;
		int m_spacing;
	};

	// Draws the part of an item's text that matches the filter on a highlight.
	class MatchDelegate : public QStyledItemDelegate
	{
	public:
		explicit MatchDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

		void setNeedle(const QString &needle) { m_needle = needle; }

		virtual void paint(QPainter *painter, const QStyleOptionViewItem &option,
			const QModelIndex &index) const
		{
			QStyledItemDelegate::paint(painter, option, index);
			if (m_needle.isEmpty())
			{
				return;
			}
			QStyleOptionViewItem opt(option);
			initStyleOption(&opt, index);
			const int at = opt.text.indexOf(m_needle, 0, Qt::CaseInsensitive);
			if (at < 0)
			{
				return;
			}
			const QWidget *widget = option.widget;
			QStyle *style = widget ? widget->style() : QApplication::style();
			const QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, widget);
			const QFontMetrics metrics(opt.font);
			const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, widget) + 1;
			const int x = textRect.left() + textMargin + metrics.horizontalAdvance(opt.text.left(at));
			const QString match = opt.text.mid(at, m_needle.length());
			const QRect box(x, textRect.top() + (textRect.height() - metrics.height()) / 2,
				metrics.horizontalAdvance(match), metrics.height());
			painter->save();
			painter->fillRect(box, QColor(215, 160, 60));
			painter->setPen(Qt::black);
			painter->setFont(opt.font);
			painter->drawText(box, Qt::AlignLeft | Qt::AlignVCenter, match);
			painter->restore();
		}

	private:
		QString m_needle;
	};
}

WBQtCondActDialog::WBQtCondActDialog(void *item, bool isAction, QWidget *parent)
	: QDialog(parent),
	m_ui(new Ui::WBQtCondActDialog),
	m_item(item),
	m_isAction(isAction ? 1 : 0),
	m_updating(false),
	m_flow(NULL)
{
	m_ui->setupUi(this);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	setWindowTitle(isAction ? "Edit Action" : "Edit Condition");
	m_ui->searchEdit->setPlaceholderText(isAction ? "Filter actions..." : "Filter conditions...");

	const int count = WBQtCondActData_GetTemplateCount(m_isAction);
	for (int i = 0; i < count; i++)
	{
		m_pathIndex.insert(templateName(m_isAction, i), i);
	}

	m_flow = new FlowLayout(m_ui->sentenceHost, 5);
	QFont sentenceFont = m_ui->sentenceHost->font();
	sentenceFont.setPointSizeF(sentenceFont.pointSizeF() + 1.0);
	m_ui->sentenceHost->setFont(sentenceFont);

	m_ui->searchEdit->installEventFilter(this);
	WBQtTreeStyle::applyTreeLines(m_ui->tree);
	m_ui->tree->setItemDelegate(new MatchDelegate(m_ui->tree));
	m_ui->split->setStretchFactor(0, 3);
	m_ui->split->setStretchFactor(1, 2);

	connect(m_ui->tree, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
			this, SLOT(onCurrentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
	connect(m_ui->tree, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onTreeContextMenu(QPoint)));
	connect(m_ui->searchEdit, SIGNAL(textChanged(QString)), this, SLOT(onFilterChanged(QString)));
	connect(m_ui->notesToggle, SIGNAL(toggled(bool)), this, SLOT(onNotesToggled(bool)));
	connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	m_ui->notesToggle->setChecked(WBQtCondAct_GetNotesOpen() != 0);
	onNotesToggled(m_ui->notesToggle->isChecked());
	applyTreeFont();

	buildTree(QString());
	renderSentence();
	showHelpForType(WBQtCondActData_GetType(m_item, m_isAction));
	m_ui->tree->setFocus();

	resize(960, 600);
}

WBQtCondActDialog::~WBQtCondActDialog()
{
	delete m_ui;
}

void WBQtCondActDialog::accept()
{
	const int type = WBQtCondActData_GetType(m_item, m_isAction);
	if (type >= 0 && type < WBQtCondActData_GetTemplateCount(m_isAction))
	{
		const QString path = templateName(m_isAction, type);
		QStringList recent = savedList(false);
		recent.removeAll(path);
		recent.prepend(path);
		while (recent.size() > kRecentMax)
		{
			recent.removeLast();
		}
		setSavedList(false, recent);
	}
	QDialog::accept();
}

bool WBQtCondActDialog::eventFilter(QObject *watched, QEvent *event)
{
	if (watched == m_ui->searchEdit && event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
		// Enter takes the first match instead of closing the dialog; with no filter it does nothing.
		if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)
		{
			if (!m_ui->searchEdit->text().trimmed().isEmpty())
			{
				selectFirstMatch();
			}
			return true;
		}
		if (keyEvent->key() == Qt::Key_Down)
		{
			m_ui->tree->setFocus();
			return true;
		}
	}
	return QDialog::eventFilter(watched, event);
}

QStringList WBQtCondActDialog::savedList(bool favorites) const
{
	static char buf[kSavedListCap];
	buf[0] = 0;
	WBQtCondAct_GetSavedList(m_isAction, favorites ? 1 : 0, buf, sizeof(buf));
	return QString::fromLocal8Bit(buf).split('|', QString::SkipEmptyParts);
}

void WBQtCondActDialog::setSavedList(bool favorites, const QStringList &paths)
{
	WBQtCondAct_SetSavedList(m_isAction, favorites ? 1 : 0, paths.join("|").toLocal8Bit().constData());
}

// The category tree from the templates' '/'-separated paths; name2 adds a second leaf. A filter
// keeps only templates whose name or name2 contains it, pruning empty folders.
void WBQtCondActDialog::buildTree(const QString &filter)
{
	m_updating = true;
	QTreeWidget *tree = m_ui->tree;
	tree->clear();

	const QStringList favorites = savedList(true);
	QHash<QString, QTreeWidgetItem *> folders;
	QTreeWidgetItem *selLeaf = NULL;
	const int curType = WBQtCondActData_GetType(m_item, m_isAction);
	const int count = WBQtCondActData_GetTemplateCount(m_isAction);
	for (int i = 0; i < count; i++)
	{
		const QString name = templateName(m_isAction, i);
		const QString name2 = templateName2(m_isAction, i);
		if (!filter.isEmpty()
			&& !name.contains(filter, Qt::CaseInsensitive)
			&& !name2.contains(filter, Qt::CaseInsensitive))
		{
			continue;
		}
		for (int pass = 0; pass < 2; pass++)
		{
			const QString path = (pass == 0) ? name : name2;
			if (path.isEmpty())
			{
				continue;
			}
			QStringList parts = path.split('/');
			const QString leafLabel = parts.takeLast();
			QTreeWidgetItem *parent = NULL;
			QString key;
			for (int p = 0; p < parts.size(); p++)
			{
				key += parts[p];
				key += '/';
				QTreeWidgetItem *folder = folders.value(key, NULL);
				if (folder == NULL)
				{
					if (parent == NULL)
					{
						folder = new QTreeWidgetItem(tree, QStringList(parts[p]));
					}
					else
					{
						folder = new QTreeWidgetItem(parent, QStringList(parts[p]));
					}
					folder->setData(0, kTemplateRole, -1);
					folders.insert(key, folder);
				}
				parent = folder;
			}
			QTreeWidgetItem *leaf;
			if (parent == NULL)
			{
				leaf = new QTreeWidgetItem(tree, QStringList(leafLabel));
			}
			else
			{
				leaf = new QTreeWidgetItem(parent, QStringList(leafLabel));
			}
			leaf->setData(0, kTemplateRole, i);
			if (favorites.contains(name))
			{
				QFont f = leaf->font(0);
				f.setItalic(true);
				leaf->setFont(0, f);
				leaf->setToolTip(0, "In Favorites");
			}
			if (pass == 0 && i == curType)
			{
				selLeaf = leaf;
			}
		}
	}
	tree->sortItems(0, Qt::AscendingOrder);

	// Saved folders go on top of the sorted catalog: Favorites first, then Recent.
	addSavedFolder("Recent", savedList(false), filter);
	addSavedFolder("Favorites", favorites, filter);

	if (!filter.isEmpty())
	{
		tree->expandAll();
	}
	static_cast<MatchDelegate *>(tree->itemDelegate())->setNeedle(filter);
	tree->viewport()->update();
	m_updating = false;
	selectCurrentType(selLeaf);
}

void WBQtCondActDialog::addSavedFolder(const QString &title, const QStringList &paths, const QString &filter)
{
	QTreeWidgetItem *folder = NULL;
	for (int p = 0; p < paths.size(); ++p)
	{
		const QHash<QString, int>::const_iterator it = m_pathIndex.constFind(paths.at(p));
		if (it == m_pathIndex.constEnd())
		{
			continue;	// a template this data set no longer has
		}
		if (!filter.isEmpty() && !paths.at(p).contains(filter, Qt::CaseInsensitive))
		{
			continue;
		}
		if (folder == NULL)
		{
			folder = new QTreeWidgetItem(QStringList(title));
			folder->setData(0, kTemplateRole, -1);
			QFont bold = folder->font(0);
			bold.setBold(true);
			folder->setFont(0, bold);
			m_ui->tree->insertTopLevelItem(0, folder);
		}
		QTreeWidgetItem *leaf = new QTreeWidgetItem(folder, QStringList(QString(paths.at(p)).replace("/", " / ")));
		leaf->setData(0, kTemplateRole, it.value());
	}
	if (folder != NULL)
	{
		folder->setExpanded(true);
	}
}

void WBQtCondActDialog::selectCurrentType(QTreeWidgetItem *leaf)
{
	if (leaf != NULL)
	{
		m_updating = true;
		m_ui->tree->setCurrentItem(leaf);
		m_ui->tree->scrollToItem(leaf, QAbstractItemView::PositionAtTop);
		m_updating = false;
	}
}

void WBQtCondActDialog::selectFirstMatch()
{
	for (QTreeWidgetItemIterator it(m_ui->tree); *it; ++it)
	{
		if ((*it)->data(0, kTemplateRole).toInt() >= 0)
		{
			m_ui->tree->setCurrentItem(*it);
			m_ui->tree->scrollToItem(*it);
			m_ui->tree->setFocus();
			return;
		}
	}
}

void WBQtCondActDialog::onCurrentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
	Q_UNUSED(previous);
	if (m_updating || current == NULL)
	{
		return;
	}
	const int type = current->data(0, kTemplateRole).toInt();
	if (type < 0)
	{
		return;
	}
	// == the TVN_SELCHANGED handler: only react when the type actually changes.
	if (type != WBQtCondActData_GetType(m_item, m_isAction))
	{
		WBQtCondActData_SetType(m_item, m_isAction, type);
		renderSentence();
		showHelpForType(type);
	}
}

void WBQtCondActDialog::onTreeContextMenu(const QPoint &pos)
{
	QTreeWidgetItem *item = m_ui->tree->itemAt(pos);
	if (item == NULL)
	{
		return;
	}
	const int type = item->data(0, kTemplateRole).toInt();
	if (type < 0)
	{
		return;
	}
	const QString path = templateName(m_isAction, type);
	QStringList favorites = savedList(true);
	const bool isFavorite = favorites.contains(path);

	QMenu menu(this);
	QAction *toggle = menu.addAction(isFavorite ? "Remove from Favorites" : "Add to Favorites");
	if (menu.exec(m_ui->tree->viewport()->mapToGlobal(pos)) != toggle)
	{
		return;
	}
	if (isFavorite)
	{
		favorites.removeAll(path);
	}
	else
	{
		favorites.append(path);
	}
	setSavedList(true, favorites);
	buildTree(m_ui->searchEdit->text().trimmed());
}

void WBQtCondActDialog::renderSentence()
{
	// Drop the previous sentence; later, since a chip's own click lands here.
	QLayoutItem *old;
	while ((old = m_flow->takeAt(0)) != NULL)
	{
		old->widget()->hide();
		old->widget()->deleteLater();
		delete old;
	}

	// The sentence interleaves uiStrings[0], param[0], uiStrings[1], param[1], ...
	const int numStrings = WBQtCondActData_GetUiStringCount(m_item, m_isAction);
	const int numParams = WBQtCondActData_GetParameterCount(m_item, m_isAction);
	const int total = (numStrings > numParams) ? numStrings : numParams;
	char buf[kTextCap];
	for (int i = 0; i < total; i++)
	{
		if (i < numStrings)
		{
			buf[0] = 0;
			WBQtCondActData_GetUiString(m_item, m_isAction, i, buf, sizeof(buf));
			const QStringList words = QString::fromLocal8Bit(buf).split(' ', QString::SkipEmptyParts);
			for (int w = 0; w < words.size(); ++w)
			{
				m_flow->addWidget(new QLabel(words.at(w), m_ui->sentenceHost));
			}
		}
		if (i < numParams)
		{
			buf[0] = 0;
			WBQtCondActData_GetParameterText(m_item, m_isAction, i, buf, sizeof(buf));
			QString text = QString::fromLocal8Bit(buf);
			if (text.isEmpty())
			{
				text = "???";
			}
			char warnBuf[kBigCap];
			warnBuf[0] = 0;
			WBQtCondActData_GetParameterWarning(m_item, m_isAction, i, warnBuf, sizeof(warnBuf));
			const QString warning = QString::fromLocal8Bit(warnBuf);

			const QColor colour = warning.isEmpty()
				? familyColour(WBQtCondActData_GetParameterFamily(m_item, m_isAction, i))
				: QColor(175, 55, 55);
			QToolButton *chip = new QToolButton(m_ui->sentenceHost);
			chip->setText(warning.isEmpty() ? text : text + "  !");
			chip->setCursor(Qt::PointingHandCursor);
			chip->setToolTip(warning.isEmpty() ? "Click to change" : warning);
			chip->setProperty("paramIndex", i);
			chip->setStyleSheet(QString(
				"QToolButton { background-color: %1; color: white; border: none; border-radius: 4px; padding: 2px 8px; }"
				"QToolButton:hover { background-color: %2; }")
				.arg(colour.name()).arg(colour.lighter(125).name()));
			connect(chip, SIGNAL(clicked()), this, SLOT(onChipClicked()));
			m_flow->addWidget(chip);
		}
	}
	updateWarnings();
}

void WBQtCondActDialog::onChipClicked()
{
	bool ok = false;
	const int index = sender()->property("paramIndex").toInt(&ok);
	if (!ok)
	{
		return;
	}
	// Pops the (still MFC) parameter editor; the sentence and warnings re-render on return.
	WBQtCondAct_EditParameter(m_item, m_isAction, index);
	renderSentence();
}

void WBQtCondActDialog::updateWarnings()
{
	char warnBuf[kBigCap];
	char infoBuf[kBigCap];
	warnBuf[0] = 0;
	infoBuf[0] = 0;
	WBQtCondActData_GetWarnings(m_item, m_isAction, warnBuf, sizeof(warnBuf), infoBuf, sizeof(infoBuf));
	const QString warnings = QString::fromLocal8Bit(warnBuf).trimmed();
	const QString information = QString::fromLocal8Bit(infoBuf).trimmed();

	QLabel *label = m_ui->warningsLabel;
	if (warnings.isEmpty() && information.isEmpty())
	{
		label->hide();
		return;
	}
	const bool warn = !warnings.isEmpty();
	const QColor accent = warn ? QColor(200, 70, 70) : QColor(80, 140, 210);
	label->setStyleSheet(QString("QLabel { border-left: 4px solid %1; background-color: rgba(%2, %3, %4, 40); padding: 6px 8px; }")
		.arg(accent.name()).arg(accent.red()).arg(accent.green()).arg(accent.blue()));
	label->setText(QString("<b>%1</b><br>%2")
		.arg(warn ? "Warnings" : "Information")
		.arg((warn ? warnings : information).toHtmlEscaped().replace("\n", "<br>")));
	label->show();
}

void WBQtCondActDialog::showHelpForType(int type)
{
	const bool valid = (type >= 0 && type < WBQtCondActData_GetTemplateCount(m_isAction));
	m_ui->helpLabel->setText(valid ? templateHelp(m_isAction, type) : QString());
}

void WBQtCondActDialog::onNotesToggled(bool open)
{
	m_ui->notesToggle->setArrowType(open ? Qt::DownArrow : Qt::RightArrow);
	m_ui->helpLabel->setVisible(open);
	WBQtCondAct_SetNotesOpen(open ? 1 : 0);
}

void WBQtCondActDialog::onFilterChanged(const QString &text)
{
	buildTree(text.trimmed());
}

void WBQtCondActDialog::applyTreeFont()
{
	// The script window's Compress Script setting: 14px compressed, 16px not.
	QFont font = m_ui->tree->font();
	font.setPixelSize(WBQtCondAct_GetCompress() ? 14 : 16);
	m_ui->tree->setFont(font);
}

// ===================== the modal entry point =====================

extern "C" int WBQtCondAct_Run(void *item, int isAction)
{
	if (item == NULL)
	{
		return 0;
	}
	// == EditCondition/EditAction::OnInitDialog clearing the item's warning flag.
	WBQtCondActData_ClearWarningFlag(item, isAction);
	// Parent to the active Qt modal (the script-edit dialog); exec() is application-modal, and
	// the MFC frame is already disabled by the outer WBQtScriptEdit_Run.
	if (WBQtScript_GetNewDesign() == 0)
	{
		WBQtCondActDialogClassic classic(item, isAction != 0, QApplication::activeModalWidget());
		return (classic.exec() == QDialog::Accepted) ? 1 : 0;
	}
	WBQtCondActDialog dlg(item, isAction != 0, QApplication::activeModalWidget());
	const int rc = dlg.exec();
	return (rc == QDialog::Accepted) ? 1 : 0;
}
