// WBQtStochasticPanel.cpp -- see WBQtStochasticPanel.h.
#include "WBQtStochasticPanel.h"
#include "WBQtStochasticBridge.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>

// MAP_XY_FACTOR from Common/MapObject.h, which the Qt lib must not include.
static const double kFeetPerCell = 10.0;

WBQtStochasticPanel *WBQtStochasticPanel::s_instance = NULL;

WBQtStochasticPanel::WBQtStochasticPanel(QWidget *owner)
	: QWidget(owner, Qt::Tool),
	  m_updating(false)
{
	setWindowTitle("Stochastic Terrain Options");
	QVBoxLayout *root = new QVBoxLayout(this);

	QPushButton *randomize = new QPushButton("Randomize");
	randomize->setToolTip("Pick a new seed for the next strokes. Seed 0 picks a new one for every stroke");
	addRow(root, "Brush Width", "Size in cells:", 1, 30, 999, m_widthSlider, m_widthSpin, &m_widthLabel);
	addRow(root, "Feather", "Width in cells:", 0, 30, 999, m_featherSlider, m_featherSpin, &m_featherLabel);
	addRow(root, "Seed", "Pattern:", 0, 255, 255, m_seedSlider, m_seedSpin, NULL, randomize);
	m_seedSpin->setSpecialValueText("Random");
	addRow(root, "Blending Rate", "Softness:", 0, 100, 100, m_rateSlider, m_rateSpin, &m_rateLabel);

	QLabel *help = new QLabel("Breaks up visible terrain tiling with shifted, turned hex cells. "
		"Strokes stamp their seed and blending rate; seed 0 gives each stroke a random one. Hold Shift to erase.");
	help->setWordWrap(true);
	root->addWidget(help);
	root->addStretch(1);

	m_updating = true;
	setRow(m_widthSlider, m_widthSpin, WBQtStochastic_GetWidth());
	setRow(m_featherSlider, m_featherSpin, WBQtStochastic_GetFeather());
	setRow(m_seedSlider, m_seedSpin, WBQtStochastic_GetSeed());
	setRow(m_rateSlider, m_rateSpin, WBQtStochastic_GetRate());
	showCaptions();
	m_updating = false;

	connect(m_widthSlider, SIGNAL(valueChanged(int)), this, SLOT(onWidthChanged(int)));
	connect(m_widthSpin, SIGNAL(valueChanged(int)), this, SLOT(onWidthChanged(int)));
	connect(m_featherSlider, SIGNAL(valueChanged(int)), this, SLOT(onFeatherChanged(int)));
	connect(m_featherSpin, SIGNAL(valueChanged(int)), this, SLOT(onFeatherChanged(int)));
	connect(m_seedSlider, SIGNAL(valueChanged(int)), this, SLOT(onSeedChanged(int)));
	connect(m_seedSpin, SIGNAL(valueChanged(int)), this, SLOT(onSeedChanged(int)));
	connect(m_rateSlider, SIGNAL(valueChanged(int)), this, SLOT(onRateChanged(int)));
	connect(m_rateSpin, SIGNAL(valueChanged(int)), this, SLOT(onRateChanged(int)));
	connect(randomize, SIGNAL(clicked()), this, SLOT(onRandomize()));

	s_instance = this;
}

WBQtStochasticPanel::~WBQtStochasticPanel()
{
	if (s_instance == this)
	{
		s_instance = NULL;
	}
}

void WBQtStochasticPanel::addRow(QVBoxLayout *root, const char *title, const char *prompt, int lo, int sliderHi, int spinHi,
	QSlider *&slider, QSpinBox *&spin, QLabel **caption, QWidget *extra)
{
	QGroupBox *box = new QGroupBox(title);
	QVBoxLayout *lay = new QVBoxLayout(box);
	QHBoxLayout *row = new QHBoxLayout();
	row->addWidget(new QLabel(prompt));
	slider = new QSlider(Qt::Horizontal);
	slider->setRange(lo, sliderHi);
	row->addWidget(slider, 1);
	spin = new QSpinBox();
	spin->setRange(lo, spinHi);
	row->addWidget(spin);
	if (extra != NULL)
	{
		row->addWidget(extra);
	}
	lay->addLayout(row);
	if (caption != NULL)
	{
		*caption = new QLabel();
		lay->addWidget(*caption);
	}
	root->addWidget(box);
}

void WBQtStochasticPanel::setRow(QSlider *slider, QSpinBox *spin, int v)
{
	// Caller has set m_updating. The spin box keeps a typed value past the slider's cap.
	slider->setValue(qBound(slider->minimum(), v, slider->maximum()));
	spin->setValue(v);
}

void WBQtStochasticPanel::showCaptions()
{
	m_widthLabel->setText(QString::asprintf("%.1f FEET.", m_widthSpin->value() * kFeetPerCell));
	m_featherLabel->setText(QString::asprintf("%.1f FEET.", m_featherSpin->value() * kFeetPerCell));
	const int rate = m_rateSpin->value();
	m_rateLabel->setText(rate < 25 ? "Visible patches" : (rate < 70 ? "Soft patches" : "Smooth mix"));
}

void WBQtStochasticPanel::onWidthChanged(int v)
{
	if (m_updating)
	{
		return;
	}
	m_updating = true;
	setRow(m_widthSlider, m_widthSpin, v);
	showCaptions();
	WBQtStochastic_SetWidth(v);	// the echo-back push is suppressed while m_updating
	m_updating = false;
}

void WBQtStochasticPanel::onFeatherChanged(int v)
{
	if (m_updating)
	{
		return;
	}
	m_updating = true;
	setRow(m_featherSlider, m_featherSpin, v);
	showCaptions();
	WBQtStochastic_SetFeather(v);
	m_updating = false;
}

void WBQtStochasticPanel::onSeedChanged(int v)
{
	if (m_updating)
	{
		return;
	}
	m_updating = true;
	setRow(m_seedSlider, m_seedSpin, v);
	WBQtStochastic_SetSeed(v);
	m_updating = false;
}

void WBQtStochasticPanel::onRateChanged(int v)
{
	if (m_updating)
	{
		return;
	}
	m_updating = true;
	setRow(m_rateSlider, m_rateSpin, v);
	showCaptions();
	WBQtStochastic_SetRate(v);
	m_updating = false;
}

void WBQtStochasticPanel::onRandomize()
{
	// The tool pushes the new seed back through pushSeed.
	WBQtStochastic_RandomizeSeed();
}

void WBQtStochasticPanel::pushWidth(int v)
{
	if (m_updating)
	{
		return;
	}
	m_updating = true;
	setRow(m_widthSlider, m_widthSpin, v);
	showCaptions();
	m_updating = false;
}

void WBQtStochasticPanel::pushFeather(int v)
{
	if (m_updating)
	{
		return;
	}
	m_updating = true;
	setRow(m_featherSlider, m_featherSpin, v);
	showCaptions();
	m_updating = false;
}

void WBQtStochasticPanel::pushSeed(int v)
{
	if (m_updating)
	{
		return;
	}
	m_updating = true;
	setRow(m_seedSlider, m_seedSpin, v);
	m_updating = false;
}

void WBQtStochasticPanel::pushRate(int v)
{
	if (m_updating)
	{
		return;
	}
	m_updating = true;
	setRow(m_rateSlider, m_rateSpin, v);
	showCaptions();
	m_updating = false;
}

// --- Forward push functions (tool -> widget), the Qt side of WBQtStochasticBridge.h -------------
extern "C" void WBQtStochastic_PushWidth(int v)
{
	if (WBQtStochasticPanel::instance() != NULL)
	{
		WBQtStochasticPanel::instance()->pushWidth(v);
	}
}

extern "C" void WBQtStochastic_PushFeather(int v)
{
	if (WBQtStochasticPanel::instance() != NULL)
	{
		WBQtStochasticPanel::instance()->pushFeather(v);
	}
}

extern "C" void WBQtStochastic_PushSeed(int v)
{
	if (WBQtStochasticPanel::instance() != NULL)
	{
		WBQtStochasticPanel::instance()->pushSeed(v);
	}
}

extern "C" void WBQtStochastic_PushRate(int v)
{
	if (WBQtStochasticPanel::instance() != NULL)
	{
		WBQtStochasticPanel::instance()->pushRate(v);
	}
}
