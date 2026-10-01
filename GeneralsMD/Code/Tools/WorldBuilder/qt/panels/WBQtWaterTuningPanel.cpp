// WBQtWaterTuningPanel.cpp -- see WBQtWaterTuningPanel.h.
#include "WBQtWaterTuningPanel.h"
#include "ui_WBQtWaterTuningPanel.h"
#include "WBQtWaterTuningBridge.h"
#include "WBQtMapIniEditorBridge.h"
#include "WBQtScrubSpinBox.h"
#include "WBQtWindowPos.h"

#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QGridLayout>
#include <QHideEvent>
#include <QLabel>
#include <QRegularExpression>
#include <QTimer>
#include <QToolButton>

WBQtWaterTuningPanel *WBQtWaterTuningPanel::s_instance = NULL;

// Defined in WBQtBridge.cpp: the main window when inverted, else an invisible
// QWinWidget bridge rooted in the MFC frame. Never hide() the result.
QWidget *WBQt_CreateOwnerBridgeWidget(void *frameHwnd);

namespace
{
	QWidget *s_owner = NULL;	// owner for the floating panel (created on first open)

	const int kSaveDelayMs = 400;
	const int kSaveRetryMs = 1500;

	// What a ticked ShaderWaterDeepColor starts from when Water.ini leaves it unset.
	const float kDeepColorStart[3] = { 40.0f, 85.0f, 95.0f };

	const char *const kChannelNames[3] = { "red", "green", "blue" };

	enum
	{
		COL_LABEL = 0,
		COL_MINUS,
		COL_SPIN,
		COL_PLUS,
		COL_RESET
	};

	int decimalsOf(double value)
	{
		const QString text = QString::number(value, 'g', 6);
		if (text.contains('e'))
		{
			return 6;
		}
		const int dot = text.indexOf('.');
		return (dot < 0) ? 0 : text.size() - dot - 1;
	}

	//----------------------------------------------------------------------------------------
	// The WaterTransparency block of a map.ini, as text.
	//----------------------------------------------------------------------------------------

	struct Block
	{
		bool found;
		int bodyStart;	// first character after the header line
		int bodyEnd;	// first character of the End line
	};

	Block findBlock(const QString &text)
	{
		static const QRegularExpression header("^[ \\t]*WaterTransparency\\b[^\\n]*\\n",
			QRegularExpression::MultilineOption);
		static const QRegularExpression end("^[ \\t]*End\\b", QRegularExpression::MultilineOption);

		Block block = { false, 0, 0 };
		const QRegularExpressionMatch head = header.match(text);
		if (!head.hasMatch())
		{
			return block;
		}
		const QRegularExpressionMatch tail = end.match(text, head.capturedEnd());
		if (!tail.hasMatch())
		{
			return block;
		}
		block.found = true;
		block.bodyStart = head.capturedEnd();
		block.bodyEnd = tail.capturedStart();
		return block;
	}

	// Captures: 1 the "Key = " lead, 2 the value, 3 the blanks before a comment or the line end.
	QRegularExpression keyPattern(const QString &key)
	{
		return QRegularExpression("^([ \\t]*" + QRegularExpression::escape(key)
			+ "[ \\t]*=[ \\t]*)([^;\\r\\n]*?)([ \\t]*)(?=;|\\r?$)",
			QRegularExpression::MultilineOption);
	}

	// The key's value text, or a null string when the block does not set it.
	QString readValue(const QString &body, const QString &key)
	{
		const QRegularExpressionMatch m = keyPattern(key).match(body);
		if (!m.hasMatch())
		{
			return QString();
		}
		const QString value = m.captured(2);
		return value.isNull() ? QString("") : value;
	}

	void writeValue(QString &body, const QString &key, const QString &value, const QString &nl)
	{
		const QRegularExpressionMatch m = keyPattern(key).match(body);
		if (m.hasMatch())
		{
			// Padding keeps a trailing comment in its column.
			QString pad;
			const int after = m.capturedEnd(3);
			if (after < body.size() && body.at(after) == ';')
			{
				const int width = m.capturedLength(2) + m.capturedLength(3) - value.size();
				pad = QString(qMax(1, width), ' ');
			}
			body.replace(m.capturedStart(2), after - m.capturedStart(2), value + pad);
			return;
		}

		// A new key takes the indent of the block's last line.
		static const QRegularExpression indented("^([ \\t]+)\\S[^\\n]*\\n?\\z",
			QRegularExpression::MultilineOption);
		const QRegularExpressionMatch last = indented.match(body);
		const QString indent = last.hasMatch() ? last.captured(1) : QString("  ");
		if (!body.isEmpty() && !body.endsWith('\n'))
		{
			body += nl;
		}
		body += indent + key + " = " + value + nl;
	}

	void removeValue(QString &body, const QString &key)
	{
		const QRegularExpression line("^[ \\t]*" + QRegularExpression::escape(key)
			+ "[ \\t]*=[^\\n]*\\n?", QRegularExpression::MultilineOption);
		const QRegularExpressionMatch m = line.match(body);
		if (m.hasMatch())
		{
			body.remove(m.capturedStart(), m.capturedLength());
		}
	}

	bool parseValue(int kind, const QString &raw, float value[3])
	{
		const QString text = raw.trimmed();
		switch (kind)
		{
			case WBQT_WATER_FLOAT:
			{
				bool ok = false;
				const float parsed = text.section(QRegularExpression("\\s"), 0, 0).toFloat(&ok);
				if (ok)
				{
					value[0] = parsed;
				}
				return ok;
			}
			case WBQT_WATER_BOOL:
			{
				if (text.compare("yes", Qt::CaseInsensitive) == 0)
				{
					value[0] = 1.0f;
					return true;
				}
				if (text.compare("no", Qt::CaseInsensitive) == 0)
				{
					value[0] = 0.0f;
					return true;
				}
				return false;
			}
			case WBQT_WATER_COLOR:
			{
				static const QRegularExpression rgb("R:\\s*(\\d+)\\s+G:\\s*(\\d+)\\s+B:\\s*(\\d+)");
				const QRegularExpressionMatch m = rgb.match(text);
				if (!m.hasMatch())
				{
					return false;
				}
				for (int c = 0; c < 3; ++c)
				{
					value[c] = m.captured(c + 1).toFloat();
				}
				return true;
			}
		}
		return false;
	}

	QToolButton *makeButton(QWidget *parent, const QString &text, int row)
	{
		QToolButton *button = new QToolButton(parent);
		button->setText(text);
		button->setFixedWidth(24);
		button->setFocusPolicy(Qt::NoFocus);
		button->setProperty("row", row);
		return button;
	}
}

WBQtWaterTuningPanel::Row::Row()
	: index(0),
	  kind(WBQT_WATER_FLOAT),
	  lo(0.0f),
	  hi(1.0f),
	  step(1.0f),
	  advanced(false),
	  inFile(false),
	  label(NULL),
	  check(NULL),
	  reset(NULL)
{
	for (int c = 0; c < 3; ++c)
	{
		value[c] = 0.0f;
		spin[c] = NULL;
		minus[c] = NULL;
		plus[c] = NULL;
	}
}

WBQtWaterTuningPanel::WBQtWaterTuningPanel(QWidget *owner)
	: QWidget(owner, Qt::Tool),
	  m_ui(new Ui::WBQtWaterTuningPanel),
	  m_saveTimer(new QTimer(this)),
	  m_updating(false)
{
	// The shell lives in WBQtWaterTuningPanel.ui; the rows come from the bridge's key table.
	m_ui->setupUi(this);
	WBQtWindowPos_Track(this, "WaterTuning");

	buildRows();
	onAdvancedToggled(m_ui->advancedCheck->isChecked());

	m_saveTimer->setSingleShot(true);
	connect(m_saveTimer, SIGNAL(timeout()), this, SLOT(onSaveTimer()));
	connect(m_ui->advancedCheck, SIGNAL(toggled(bool)), this, SLOT(onAdvancedToggled(bool)));

	s_instance = this;
}

WBQtWaterTuningPanel::~WBQtWaterTuningPanel()
{
	if (s_instance == this)
	{
		s_instance = NULL;
	}
	delete m_ui;
}

void WBQtWaterTuningPanel::addStepper(Row &row, int rowIndex, int channel, int gridRow, const QString &tip)
{
	QWidget *host = m_ui->rowsHost;
	QGridLayout *grid = m_ui->rowsGrid;

	QToolButton *minus = makeButton(host, QString(QChar(0x2212)), rowIndex);
	QToolButton *plus = makeButton(host, "+", rowIndex);
	minus->setProperty("chan", channel);
	minus->setProperty("dir", -1);
	plus->setProperty("chan", channel);
	plus->setProperty("dir", 1);
	minus->setAutoRepeat(true);
	plus->setAutoRepeat(true);

	WBQtScrubSpinBox *spin = new WBQtScrubSpinBox(host);
	spin->setButtonSymbols(QAbstractSpinBox::NoButtons);
	spin->setKeyboardTracking(false);
	spin->setAlignment(Qt::AlignRight);
	spin->setMinimumWidth(72);
	spin->setToolTip(tip);
	spin->setProperty("row", rowIndex);
	spin->setProperty("chan", channel);

	grid->addWidget(minus, gridRow, COL_MINUS);
	grid->addWidget(spin, gridRow, COL_SPIN);
	grid->addWidget(plus, gridRow, COL_PLUS);

	connect(minus, SIGNAL(clicked()), this, SLOT(onStepClicked()));
	connect(plus, SIGNAL(clicked()), this, SLOT(onStepClicked()));
	connect(spin, SIGNAL(valueChanged(double)), this, SLOT(onSpinChanged(double)));

	row.minus[channel] = minus;
	row.plus[channel] = plus;
	row.spin[channel] = spin;
	row.widgets << minus << spin << plus;
}

void WBQtWaterTuningPanel::buildRows()
{
	QWidget *host = m_ui->rowsHost;
	QGridLayout *grid = m_ui->rowsGrid;
	int gridRow = 0;

	const int count = WBQtWaterTuning_Count();
	m_rows.reserve(count);
	for (int i = 0; i < count; ++i)
	{
		WBQtWaterTuningDesc desc;
		if (!WBQtWaterTuning_GetDesc(i, &desc))
		{
			continue;
		}
		const int r = m_rows.size();
		const QString tip = QString::fromLatin1(desc.help);

		Row row;
		row.index = i;
		row.kind = desc.kind;
		row.key = QString::fromLatin1(desc.key);
		row.lo = desc.lo;
		row.hi = desc.hi;
		row.step = desc.step;
		row.advanced = desc.advanced != 0;

		row.reset = makeButton(host, QString(QChar(0x21BA)), r);
		row.reset->setToolTip(tr("Remove the key from map.ini, so the map follows Water.ini"));
		connect(row.reset, SIGNAL(clicked()), this, SLOT(onResetClicked()));
		grid->addWidget(row.reset, gridRow, COL_RESET);
		row.widgets << row.reset;

		if (desc.kind == WBQT_WATER_FLOAT)
		{
			row.label = new QLabel(row.key, host);
			row.label->setToolTip(tip);
			grid->addWidget(row.label, gridRow, COL_LABEL);
			row.widgets << row.label;

			addStepper(row, r, 0, gridRow, tip);
			// Wider than lo to hi, so a typed or loaded value is never clamped.
			row.spin[0]->setRange((desc.lo < 0.0f) ? desc.lo * 10.0 - 10.0 : 0.0, desc.hi * 10.0 + 10.0);
			row.spin[0]->setSingleStep(desc.step);
			row.spin[0]->setDecimals(decimalsOf(desc.step));
			++gridRow;
		}
		else
		{
			row.check = new QCheckBox(row.key, host);
			row.check->setToolTip(tip);
			row.check->setProperty("row", r);
			connect(row.check, SIGNAL(toggled(bool)), this, SLOT(onCheckToggled(bool)));
			grid->addWidget(row.check, gridRow, COL_LABEL, 1, COL_RESET - COL_LABEL);
			row.widgets << row.check;
			++gridRow;

			if (desc.kind == WBQT_WATER_COLOR)
			{
				for (int c = 0; c < 3; ++c)
				{
					QLabel *channel = new QLabel(QString("    ") + kChannelNames[c], host);
					channel->setToolTip(tip);
					grid->addWidget(channel, gridRow, COL_LABEL);
					row.widgets << channel;

					addStepper(row, r, c, gridRow, tip);
					row.spin[c]->setRange(0.0, 255.0);
					row.spin[c]->setSingleStep(1.0);
					row.spin[c]->setDecimals(0);
					++gridRow;
				}
			}
		}
		m_rows.append(row);
	}
	grid->setRowStretch(gridRow, 1);
}

QString WBQtWaterTuningPanel::formatValue(const Row &row) const
{
	switch (row.kind)
	{
		case WBQT_WATER_BOOL:
			return (row.value[0] >= 0.5f) ? "Yes" : "No";
		case WBQT_WATER_COLOR:
			return QString("R:%1 G:%2 B:%3")
				.arg(qBound(0, qRound(row.value[0]), 255))
				.arg(qBound(0, qRound(row.value[1]), 255))
				.arg(qBound(0, qRound(row.value[2]), 255));
		default:
			return QString::number(row.value[0], 'g', 6);
	}
}

void WBQtWaterTuningPanel::setStatus(const QString &text)
{
	m_ui->statusLabel->setText(text);
}

// Put a row's value and its "set in map.ini" state on its widgets.
void WBQtWaterTuningPanel::showRow(int r)
{
	const bool wasUpdating = m_updating;
	m_updating = true;

	Row &row = m_rows[r];
	QWidget *title = (row.label != NULL) ? static_cast<QWidget *>(row.label) : row.check;
	QFont font = title->font();
	font.setBold(row.inFile);
	title->setFont(font);
	row.reset->setEnabled(row.inFile);

	switch (row.kind)
	{
		case WBQT_WATER_FLOAT:
		{
			// A loaded value keeps the decimals it was written with.
			row.spin[0]->setDecimals(qMax(decimalsOf(row.step), decimalsOf(row.value[0])));
			row.spin[0]->setValue(row.value[0]);
			break;
		}
		case WBQT_WATER_BOOL:
		{
			row.check->setChecked(row.value[0] >= 0.5f);
			break;
		}
		case WBQT_WATER_COLOR:
		{
			row.check->setChecked(row.inFile);
			for (int c = 0; c < 3; ++c)
			{
				row.spin[c]->setValue((row.value[c] < 0.0f) ? kDeepColorStart[c] : row.value[c]);
				row.spin[c]->setEnabled(row.inFile);
				row.minus[c]->setEnabled(row.inFile);
				row.plus[c]->setEnabled(row.inFile);
			}
			break;
		}
	}

	m_updating = wasUpdating;
}

// The user changed a row: show it in the 3D view and queue it for map.ini.
void WBQtWaterTuningPanel::markChanged(int r)
{
	Row &row = m_rows[r];
	row.inFile = true;
	showRow(r);
	WBQtWaterTuning_SetLive(row.index, row.value);
	m_pending[row.key] = formatValue(row);
	m_saveTimer->start(kSaveDelayMs);
}

// Take the key out of map.ini and return the row to the Water.ini value.
void WBQtWaterTuningPanel::resetRow(int r)
{
	Row &row = m_rows[r];
	WBQtWaterTuning_GetBase(row.index, row.value);
	row.inFile = false;
	showRow(r);
	WBQtWaterTuning_SetLive(row.index, row.value);
	m_pending[row.key] = QString();
	m_saveTimer->start(kSaveDelayMs);
}

void WBQtWaterTuningPanel::openOn(const QString &iniPath)
{
	flush();
	if (iniPath != m_path)
	{
		m_pending.clear();
	}
	m_path = iniPath;
	reseed();
	setStatus(QDir::toNativeSeparators(m_path));
}

void WBQtWaterTuningPanel::reseed()
{
	QString body;
	QFile file(m_path);
	if (!m_path.isEmpty() && file.open(QIODevice::ReadOnly))
	{
		const QString text = QString::fromLatin1(file.readAll());
		file.close();
		const Block block = findBlock(text);
		if (block.found)
		{
			body = text.mid(block.bodyStart, block.bodyEnd - block.bodyStart);
		}
	}

	// Edits a failed save left pending win over the file, and stay queued.
	for (int r = 0; r < m_rows.size(); ++r)
	{
		Row &row = m_rows[r];
		WBQtWaterTuning_GetBase(row.index, row.value);
		const QString raw = m_pending.contains(row.key) ? m_pending.value(row.key) : readValue(body, row.key);
		row.inFile = !raw.isNull() && parseValue(row.kind, raw, row.value);
		showRow(r);
		WBQtWaterTuning_SetLive(row.index, row.value);
	}
	if (!m_pending.isEmpty())
	{
		m_saveTimer->start(kSaveRetryMs);
	}
}

bool WBQtWaterTuningPanel::flush()
{
	m_saveTimer->stop();
	if (m_pending.isEmpty() || m_path.isEmpty())
	{
		m_pending.clear();
		return true;
	}

	const QByteArray localPath = m_path.toLocal8Bit();
	if (WBQtMapIniEditor_HasUnsavedChanges(localPath.constData()))
	{
		setStatus(tr("Not saved: map.ini has unsaved edits in the map.ini editor."));
		return false;
	}

	QString text;
	QFile file(m_path);
	if (file.exists())
	{
		if (!file.open(QIODevice::ReadOnly))
		{
			setStatus(tr("Not saved: could not read %1.").arg(QDir::toNativeSeparators(m_path)));
			return false;
		}
		text = QString::fromLatin1(file.readAll());
		file.close();
	}
	const QString nl = (text.isEmpty() || text.contains("\r\n")) ? "\r\n" : "\n";

	bool anyWrite = false;
	QMap<QString, QString>::const_iterator it;
	for (it = m_pending.constBegin(); it != m_pending.constEnd(); ++it)
	{
		if (!it.value().isNull())
		{
			anyWrite = true;
		}
	}

	Block block = findBlock(text);
	if (!block.found)
	{
		if (!anyWrite)
		{
			m_pending.clear();
			return true;
		}
		if (!text.isEmpty())
		{
			text += text.endsWith('\n') ? nl : nl + nl;
		}
		text += "WaterTransparency" + nl;
		block.bodyStart = text.size();
		block.bodyEnd = text.size();
		text += "End" + nl;
	}

	QString body = text.mid(block.bodyStart, block.bodyEnd - block.bodyStart);
	for (it = m_pending.constBegin(); it != m_pending.constEnd(); ++it)
	{
		if (it.value().isNull())
		{
			removeValue(body, it.key());
		}
		else
		{
			writeValue(body, it.key(), it.value(), nl);
		}
	}
	text.replace(block.bodyStart, block.bodyEnd - block.bodyStart, body);

	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		setStatus(tr("Not saved: could not write %1.").arg(QDir::toNativeSeparators(m_path)));
		return false;
	}
	file.write(text.toLatin1());
	file.close();

	m_pending.clear();
	WBQtWaterTuning_NoteSaved();
	WBQtMapIniEditor_ReloadIfOpen(localPath.constData());
	setStatus(tr("Saved to %1.").arg(QDir::toNativeSeparators(m_path)));
	return true;
}

void WBQtWaterTuningPanel::mapChanged()
{
	flush();
	m_pending.clear();
	m_path.clear();
	hide();
}

void WBQtWaterTuningPanel::hideEvent(QHideEvent *event)
{
	flush();
	QWidget::hideEvent(event);
}

void WBQtWaterTuningPanel::onSpinChanged(double value)
{
	if (m_updating)
	{
		return;
	}
	const int r = sender()->property("row").toInt();
	const int c = sender()->property("chan").toInt();
	m_rows[r].value[c] = (float)value;
	markChanged(r);
}

void WBQtWaterTuningPanel::onStepClicked()
{
	const int r = sender()->property("row").toInt();
	const int c = sender()->property("chan").toInt();
	const int dir = sender()->property("dir").toInt();
	const Row &row = m_rows[r];
	WBQtScrubSpinBox *spin = row.spin[c];

	// A value already past lo or hi may step back in, but not further out.
	const double now = spin->value();
	double next = now + dir * spin->singleStep();
	if (dir > 0 && next > row.hi)
	{
		next = qMax(now, (double)row.hi);
	}
	if (dir < 0 && next < row.lo)
	{
		next = qMin(now, (double)row.lo);
	}
	spin->setValue(next);
}

void WBQtWaterTuningPanel::onCheckToggled(bool on)
{
	if (m_updating)
	{
		return;
	}
	const int r = sender()->property("row").toInt();
	Row &row = m_rows[r];
	if (row.kind == WBQT_WATER_BOOL)
	{
		row.value[0] = on ? 1.0f : 0.0f;
		markChanged(r);
		return;
	}

	// The colour's box says whether map.ini sets the key at all.
	if (!on)
	{
		resetRow(r);
		return;
	}
	for (int c = 0; c < 3; ++c)
	{
		row.value[c] = (float)row.spin[c]->value();
	}
	markChanged(r);
}

void WBQtWaterTuningPanel::onResetClicked()
{
	resetRow(sender()->property("row").toInt());
}

void WBQtWaterTuningPanel::onAdvancedToggled(bool on)
{
	for (int r = 0; r < m_rows.size(); ++r)
	{
		const Row &row = m_rows[r];
		if (!row.advanced)
		{
			continue;
		}
		for (int w = 0; w < row.widgets.size(); ++w)
		{
			row.widgets[w]->setVisible(on);
		}
	}
}

void WBQtWaterTuningPanel::onSaveTimer()
{
	if (!flush() && isVisible())
	{
		m_saveTimer->start(kSaveRetryMs);
	}
}

// --- Open / map change / refresh hooks (the Qt side of WBQtWaterTuningBridge.h) ---------------

extern "C" void WBQtWaterTuning_Open(void *frameHwnd, const char *iniPath)
{
	if (frameHwnd == NULL || iniPath == NULL)
	{
		return;
	}
	if (s_owner == NULL)
	{
		s_owner = WBQt_CreateOwnerBridgeWidget(frameHwnd);
	}
	WBQtWaterTuningPanel *panel = WBQtWaterTuningPanel::instance();
	if (panel == NULL)
	{
		panel = new WBQtWaterTuningPanel(s_owner);
	}
	panel->openOn(QString::fromLocal8Bit(iniPath));
	// Show WITHOUT activating (== the MFC SW_SHOWNA) so the viewport keeps focus.
	panel->setAttribute(Qt::WA_ShowWithoutActivating);
	panel->show();
	panel->raise();
}

extern "C" void WBQtWaterTuning_MapChanged(void)
{
	WBQtWaterTuningPanel *panel = WBQtWaterTuningPanel::instance();
	if (panel != NULL)
	{
		panel->mapChanged();
	}
}

extern "C" void WBQtWaterTuning_PushRefresh(void)
{
	WBQtWaterTuningPanel *panel = WBQtWaterTuningPanel::instance();
	if (panel != NULL && panel->isVisible())
	{
		panel->flush();
		panel->reseed();
	}
}
