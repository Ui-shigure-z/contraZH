// WBQtStochasticPanel.h -- Qt options panel for the Stochastic Terrain brush.
//
// A top-level Qt::Tool window owned by the shared QWinWidget bridge (see WBQtOptionsPanels.cpp),
// laid out like WBQtBrushPanel: a group per value with a slider and spin box kept in step. Its
// controls drive StochasticTool through WBQtStochasticBridge.h, and the tool pushes changes back
// through the WBQtStochastic_Push* functions defined in the .cpp. There is no MFC twin.
#ifndef WB_QT_STOCHASTIC_PANEL_H
#define WB_QT_STOCHASTIC_PANEL_H

#include <QWidget>

class QSlider;
class QSpinBox;
class QLabel;
class QVBoxLayout;

class WBQtStochasticPanel : public QWidget
{
	Q_OBJECT

public:
	explicit WBQtStochasticPanel(QWidget *owner);
	virtual ~WBQtStochasticPanel();

	// Tool -> widget display push (called by the free WBQtStochastic_Push* functions).
	void pushWidth(int v);
	void pushFeather(int v);
	void pushSeed(int v);
	void pushRate(int v);

	static WBQtStochasticPanel *instance() { return s_instance; }

private slots:
	void onWidthChanged(int v);
	void onFeatherChanged(int v);
	void onSeedChanged(int v);
	void onRateChanged(int v);
	void onRandomize();

private:
	/// A titled group holding a slider and a spin box, and a caption line under them when caption is set.
	void addRow(QVBoxLayout *root, const char *title, const char *prompt, int lo, int sliderHi, int spinHi,
		QSlider *&slider, QSpinBox *&spin, QLabel **caption, QWidget *extra = NULL);
	void setRow(QSlider *slider, QSpinBox *spin, int v);	// set both without re-entry
	void showCaptions();

	QSlider  *m_widthSlider;
	QSpinBox *m_widthSpin;
	QLabel   *m_widthLabel;
	QSlider  *m_featherSlider;
	QSpinBox *m_featherSpin;
	QLabel   *m_featherLabel;
	QSlider  *m_seedSlider;
	QSpinBox *m_seedSpin;
	QSlider  *m_rateSlider;
	QSpinBox *m_rateSpin;
	QLabel   *m_rateLabel;

	bool m_updating;	// re-entrancy guard, as in WBQtBrushPanel

	static WBQtStochasticPanel *s_instance;
};

#endif // WB_QT_STOCHASTIC_PANEL_H
