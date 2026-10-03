// WBQtHQPreviewDialog.cpp -- shows the HQ map preview and its controls before the tga is
// written. See WBQtHQPreviewBridge.h.
#include "WBQtHQPreviewBridge.h"

#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>

#include <vector>

// Modal-dialog parent (active modal if nested, else main window). WBQtBridge.cpp.
QWidget *WBQt_DialogParent(void);

namespace
{

const int kDisplaySize = 512;

class WBQtHQPreviewDialog : public QDialog
{
public:
	WBQtHQPreviewDialog(const QString &tgaPath, QWidget *parent)
		: QDialog(parent)
	{
		setWindowTitle(tr("HQ Map Preview"));

		m_image = new QLabel(this);
		m_image->setFixedSize(kDisplaySize, kDisplaySize);
		m_image->setAlignment(Qt::AlignCenter);
		m_image->setFrameShape(QFrame::Box);

		QVBoxLayout *controls = new QVBoxLayout();
		controls->addWidget(buildShadingBox());
		controls->addWidget(buildRenderBox());

		QPushButton *resetBtn = new QPushButton(tr("Defaults"), this);
		resetBtn->setToolTip(tr("Puts every control back to its default. Render settings that change need a new render."));
		connect(resetBtn, &QPushButton::clicked, this, [this]()
		{
			WBQtHQPreviewParams params;
			WBQtHQCaptureParams capture;
			WBQtHQPreview_GetDefaults(&params, &capture);
			loadCapture(capture);
			loadShading(params);
			updateRenderState();
		});
		controls->addWidget(resetBtn, 0, Qt::AlignLeft);
		controls->addStretch(1);

		QLabel *pathLabel = new QLabel(tgaPath, this);
		pathLabel->setWordWrap(true);
		pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
		controls->addWidget(pathLabel);

		QHBoxLayout *body = new QHBoxLayout();
		body->addWidget(m_image, 0, Qt::AlignTop);
		body->addLayout(controls, 1);

		QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
		connect(buttons, &QDialogButtonBox::accepted, this, [this]()
		{
			if (m_renderDirty && !render())
			{
				return;
			}
			const WBQtHQPreviewParams params = shading();
			const WBQtHQCaptureParams capture = captureSettings();
			if (WBQtHQPreview_Save(&params, &capture) != 0)
			{
				accept();
			}
			else
			{
				QMessageBox::warning(this, windowTitle(), tr("Couldn't write the tga (is the map folder writable?)."));
			}
		});
		connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

		QVBoxLayout *root = new QVBoxLayout(this);
		root->addLayout(body);
		root->addWidget(buttons);

		// The bridge rendered with the last settings before the dialog opened.
		WBQtHQPreviewParams params;
		WBQtHQCaptureParams capture;
		WBQtHQPreview_GetLast(&params, &capture);
		loadCapture(capture);
		m_rendered = capture;
		loadShading(params);
		updateRenderState();
	}

private:
	QGroupBox *buildShadingBox()
	{
		QGroupBox *box = new QGroupBox(tr("Shading"), this);
		QGridLayout *grid = new QGridLayout(box);
		m_relief = addSlider(grid, 0, tr("Relief"), 0, 200, 1000, true,
			tr("Strength of the hillshade that lights slopes from the north-west. Type past the slider for more."));
		m_elevation = addSlider(grid, 1, tr("Elevation"), 0, 100, 500, true,
			tr("How much brighter high ground is than low ground. Type past the slider for more."));
		m_falloff = addSlider(grid, 2, tr("Water depth"), 5, 200, 5000, false,
			tr("Depth in world units at which water reaches most of its deep colour. Type past the slider for more."));
		m_shallowBtn = addColorButton(grid, 3, tr("Shallow water"), m_shallow);
		m_deepBtn = addColorButton(grid, 4, tr("Deep water"), m_deep);
		m_depthTint = new QCheckBox(tr("Depth tint"), box);
		m_depthTint->setToolTip(tr("Colours water from shallow to deep by its depth, over whatever the render drew."));
		grid->addWidget(m_depthTint, 5, 0, 1, 3);
		connect(m_depthTint, &QCheckBox::toggled, this, [this]() { refresh(); });
		return box;
	}

	QGroupBox *buildRenderBox()
	{
		QGroupBox *box = new QGroupBox(tr("Render"), this);
		QGridLayout *grid = new QGridLayout(box);
		int row = 0;

		m_objects = new QCheckBox(tr("Objects"), box);
		m_trees = new QCheckBox(tr("Trees"), box);
		m_roads = new QCheckBox(tr("Roads and bridges"), box);
		m_colorGrade = new QCheckBox(tr("Colour grade"), box);
		m_colorGrade->setToolTip(tr("Applies the map.ini colour grade, as in the game. Water is tinted after it."));
		grid->addWidget(m_objects, row, 0);
		grid->addWidget(m_trees, row, 1);
		row++;
		grid->addWidget(m_roads, row, 0);
		grid->addWidget(m_colorGrade, row, 1);
		row++;

		m_renderedWater = new QCheckBox(tr("Rendered water"), box);
		m_renderedWater->setToolTip(tr("Draws the flat water surface with the map's water texture. Bridges cover it."));
		m_shaderWater = new QCheckBox(tr("Shader water"), box);
		m_shaderWater->setToolTip(tr("Draws the shader water, as in the game, in place of the flat surface. Seen from straight above it shows little reflection."));
		grid->addWidget(m_renderedWater, row, 0);
		grid->addWidget(m_shaderWater, row, 1);
		row++;

		m_timeOfDay = new QComboBox(box);
		m_timeOfDay->addItem(tr("Current"));
		m_timeOfDay->addItem(tr("Morning"));
		m_timeOfDay->addItem(tr("Afternoon"));
		m_timeOfDay->addItem(tr("Evening"));
		m_timeOfDay->addItem(tr("Night"));
		m_timeOfDay->setToolTip(tr("The lighting to render with. Current keeps the editor's time of day."));
		grid->addWidget(new QLabel(tr("Time of day"), box), row, 0);
		grid->addWidget(m_timeOfDay, row, 1);
		row++;

		m_area = new QComboBox(box);
		m_area->addItem(tr("Whole map"));
		m_area->addItem(tr("Playable area"));
		m_area->addItem(tr("Custom"));
		grid->addWidget(new QLabel(tr("Area"), box), row, 0);
		grid->addWidget(m_area, row, 1);
		row++;

		int mapWidth = 0;
		int mapHeight = 0;
		int playableWidth = 0;
		int playableHeight = 0;
		WBQtHQPreview_GetMapCells(&mapWidth, &mapHeight, &playableWidth, &playableHeight);
		QHBoxLayout *custom = new QHBoxLayout();
		m_x0 = addCellSpin(custom, tr("X"), mapWidth);
		m_y0 = addCellSpin(custom, tr("Y"), mapHeight);
		m_x1 = addCellSpin(custom, tr("to X"), mapWidth);
		m_y1 = addCellSpin(custom, tr("Y"), mapHeight);
		grid->addLayout(custom, row, 0, 1, 2);
		row++;

		m_areaWarning = new QLabel(tr("The lobby places start positions across the whole map, so they will not line up with this preview."), box);
		m_areaWarning->setWordWrap(true);
		m_areaWarning->setStyleSheet("color: #b06000;");
		grid->addWidget(m_areaWarning, row, 0, 1, 2);
		row++;

		m_size = new QComboBox(box);
		m_size->addItem("128", 128);
		m_size->addItem("256", 256);
		m_size->addItem("512", 512);
		m_size->setToolTip(tr("Pixels per side of the tga."));
		grid->addWidget(new QLabel(tr("Output size"), box), row, 0);
		grid->addWidget(m_size, row, 1);
		row++;

		m_supersample = new QComboBox(box);
		m_supersample->addItem("2x", 2);
		m_supersample->addItem("4x", 4);
		m_supersample->addItem("8x", 8);
		m_supersample->setToolTip(tr("Renders this many pixels per output pixel and averages them. Higher smooths edges but renders slower."));
		grid->addWidget(new QLabel(tr("Supersampling"), box), row, 0);
		grid->addWidget(m_supersample, row, 1);
		row++;

		m_renderInfo = new QLabel(box);
		m_renderInfo->setWordWrap(true);
		grid->addWidget(m_renderInfo, row, 0, 1, 2);
		row++;

		m_renderBtn = new QPushButton(tr("Render"), box);
		connect(m_renderBtn, &QPushButton::clicked, this, [this]()
		{
			render();
		});
		grid->addWidget(m_renderBtn, row, 0, 1, 2, Qt::AlignLeft);

		QCheckBox *checks[] = { m_objects, m_trees, m_roads, m_colorGrade, m_renderedWater, m_shaderWater };
		for (int i = 0; i < 6; i++)
		{
			connect(checks[i], &QCheckBox::toggled, this, [this]() { updateRenderState(); });
		}
		QComboBox *combos[] = { m_timeOfDay, m_area, m_size, m_supersample };
		for (int i = 0; i < 4; i++)
		{
			connect(combos[i], QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() { updateRenderState(); });
		}
		QSpinBox *spins[] = { m_x0, m_y0, m_x1, m_y1 };
		for (int i = 0; i < 4; i++)
		{
			connect(spins[i], QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() { updateRenderState(); });
		}
		return box;
	}

	// The spin box holds the value and takes typed values past the slider's range, which the slider then pins at its end.
	QSpinBox *addSlider(QGridLayout *grid, int row, const QString &label, int lo, int hi, int typedMax, bool percent, const QString &help)
	{
		QLabel *name = new QLabel(label, this);
		QSlider *slider = new QSlider(Qt::Horizontal, this);
		QSpinBox *value = new QSpinBox(this);
		slider->setRange(lo, hi);
		value->setRange(lo, typedMax);
		if (percent)
		{
			value->setSuffix("%");
		}
		slider->setMinimumWidth(160);
		value->setMinimumWidth(70);
		name->setToolTip(help);
		slider->setToolTip(help);
		value->setToolTip(help);
		grid->addWidget(name, row, 0);
		grid->addWidget(slider, row, 1);
		grid->addWidget(value, row, 2);
		connect(slider, &QSlider::valueChanged, this, [value](int v)
		{
			value->setValue(v);
		});
		connect(value, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, slider](int v)
		{
			const QSignalBlocker block(slider);
			slider->setValue(v);
			refresh();
		});
		return value;
	}

	QPushButton *addColorButton(QGridLayout *grid, int row, const QString &label, QColor &color)
	{
		QPushButton *button = new QPushButton(this);
		button->setFixedSize(60, 22);
		grid->addWidget(new QLabel(label, this), row, 0);
		grid->addWidget(button, row, 1, Qt::AlignLeft);
		connect(button, &QPushButton::clicked, this, [this, button, label, &color]()
		{
			const QColor picked = QColorDialog::getColor(color, this, label);
			if (picked.isValid())
			{
				color = picked;
				paintSwatch(button, color);
				refresh();
			}
		});
		return button;
	}

	QSpinBox *addCellSpin(QHBoxLayout *row, const QString &label, int maximum)
	{
		QSpinBox *spin = new QSpinBox(this);
		spin->setRange(0, maximum);
		spin->setToolTip(tr("Cells from the map's south-west corner, without the border."));
		row->addWidget(new QLabel(label, this));
		row->addWidget(spin);
		return spin;
	}

	static void paintSwatch(QPushButton *button, const QColor &color)
	{
		button->setStyleSheet(QString("background-color: %1; border: 1px solid #555;").arg(color.name()));
	}

	static void selectData(QComboBox *combo, int value)
	{
		const int index = combo->findData(value);
		if (index >= 0)
		{
			combo->setCurrentIndex(index);
		}
	}

	void loadShading(const WBQtHQPreviewParams &params)
	{
		m_loading = true;
		m_relief->setValue((int)(params.relief*100.0f + 0.5f));
		m_elevation->setValue((int)(params.elevation*100.0f + 0.5f));
		m_falloff->setValue((int)(params.waterFalloff + 0.5f));
		m_depthTint->setChecked(params.depthTint != 0);
		m_shallow = QColor(params.shallow[0], params.shallow[1], params.shallow[2]);
		m_deep = QColor(params.deep[0], params.deep[1], params.deep[2]);
		paintSwatch(m_shallowBtn, m_shallow);
		paintSwatch(m_deepBtn, m_deep);
		m_loading = false;
		refresh();
	}

	void loadCapture(const WBQtHQCaptureParams &capture)
	{
		m_loading = true;
		m_objects->setChecked(capture.objects != 0);
		m_trees->setChecked(capture.trees != 0);
		m_roads->setChecked(capture.roads != 0);
		m_colorGrade->setChecked(capture.colorGrade != 0);
		m_renderedWater->setChecked(capture.renderedWater != 0);
		m_shaderWater->setChecked(capture.shaderWater != 0);
		m_timeOfDay->setCurrentIndex(qBound(0, capture.timeOfDay, m_timeOfDay->count() - 1));
		m_area->setCurrentIndex(qBound(0, capture.area, m_area->count() - 1));
		m_x0->setValue(capture.customX0);
		m_y0->setValue(capture.customY0);
		m_x1->setValue(capture.customX1);
		m_y1->setValue(capture.customY1);
		selectData(m_size, capture.size);
		selectData(m_supersample, capture.supersample);
		m_loading = false;
	}

	WBQtHQPreviewParams shading() const
	{
		WBQtHQPreviewParams params;
		params.relief = m_relief->value() / 100.0f;
		params.elevation = m_elevation->value() / 100.0f;
		params.waterFalloff = (float)m_falloff->value();
		params.depthTint = m_depthTint->isChecked() ? 1 : 0;
		params.shallow[0] = m_shallow.red();
		params.shallow[1] = m_shallow.green();
		params.shallow[2] = m_shallow.blue();
		params.deep[0] = m_deep.red();
		params.deep[1] = m_deep.green();
		params.deep[2] = m_deep.blue();
		return params;
	}

	WBQtHQCaptureParams captureSettings() const
	{
		WBQtHQCaptureParams capture;
		capture.objects = m_objects->isChecked() ? 1 : 0;
		capture.trees = m_trees->isChecked() ? 1 : 0;
		capture.roads = m_roads->isChecked() ? 1 : 0;
		capture.colorGrade = m_colorGrade->isChecked() ? 1 : 0;
		capture.renderedWater = m_renderedWater->isChecked() ? 1 : 0;
		capture.shaderWater = m_shaderWater->isChecked() ? 1 : 0;
		capture.timeOfDay = m_timeOfDay->currentIndex();
		capture.area = m_area->currentIndex();
		capture.customX0 = m_x0->value();
		capture.customY0 = m_y0->value();
		capture.customX1 = m_x1->value();
		capture.customY1 = m_y1->value();
		capture.size = m_size->currentData().toInt();
		capture.supersample = m_supersample->currentData().toInt();
		return capture;
	}

	static bool sameCapture(const WBQtHQCaptureParams &a, const WBQtHQCaptureParams &b)
	{
		const bool sameCustom = a.area != WBQT_HQ_AREA_CUSTOM || (a.customX0 == b.customX0 && a.customY0 == b.customY0
			&& a.customX1 == b.customX1 && a.customY1 == b.customY1);
		return a.objects == b.objects && a.trees == b.trees && a.roads == b.roads && a.colorGrade == b.colorGrade
			&& a.renderedWater == b.renderedWater && a.shaderWater == b.shaderWater
			&& a.timeOfDay == b.timeOfDay && a.area == b.area && sameCustom && a.size == b.size && a.supersample == b.supersample;
	}

	void updateRenderState()
	{
		if (m_loading)
		{
			return;
		}
		const WBQtHQCaptureParams capture = captureSettings();
		const bool custom = capture.area == WBQT_HQ_AREA_CUSTOM;
		m_x0->setEnabled(custom);
		m_y0->setEnabled(custom);
		m_x1->setEnabled(custom);
		m_y1->setEnabled(custom);
		m_areaWarning->setVisible(capture.area != WBQT_HQ_AREA_MAP);
		m_renderedWater->setEnabled(!m_shaderWater->isChecked());

		m_renderDirty = !sameCapture(capture, m_rendered);

		const int maxCapture = WBQtHQPreview_MaxCapture();
		const int effective = qMax(1, qMin(capture.supersample, maxCapture / qMax(1, capture.size)));
		QString info = tr("Renders %1 x %1 pixels.").arg(capture.size*effective);
		if (effective < capture.supersample)
		{
			info += " " + tr("Supersampling is reduced to %1x to stay within %2 pixels.").arg(effective).arg(maxCapture);
		}
		if (m_renderDirty)
		{
			info += " " + tr("Settings changed. Press Render to update the image.");
		}
		m_renderInfo->setText(info);
	}

	bool render()
	{
		const WBQtHQCaptureParams capture = captureSettings();
		QApplication::setOverrideCursor(Qt::WaitCursor);
		const int ok = WBQtHQPreview_Render(&capture);
		QApplication::restoreOverrideCursor();
		if (ok == 0)
		{
			char reason[256];
			WBQtHQPreview_GetError(reason, sizeof(reason));
			QMessageBox::warning(this, windowTitle(), tr("Couldn't render the map: %1.").arg(QString::fromLocal8Bit(reason)));
			return false;
		}
		m_rendered = capture;
		updateRenderState();
		refresh();
		return true;
	}

	void refresh()
	{
		if (m_loading)
		{
			return;
		}
		const WBQtHQPreviewParams params = shading();
		const int size = WBQtHQPreview_Size();
		m_pixels.resize(size*size*4);
		WBQtHQPreview_Compose(&params, &m_pixels[0]);
		QImage image(&m_pixels[0], size, size, size*4, QImage::Format_RGB32);
		m_image->setPixmap(QPixmap::fromImage(image.scaled(kDisplaySize, kDisplaySize,
			Qt::IgnoreAspectRatio, size < kDisplaySize ? Qt::FastTransformation : Qt::SmoothTransformation)));
	}

	std::vector<unsigned char> m_pixels;
	QLabel *m_image;

	QSpinBox *m_relief;
	QSpinBox *m_elevation;
	QSpinBox *m_falloff;
	QPushButton *m_shallowBtn;
	QPushButton *m_deepBtn;
	QColor m_shallow;
	QColor m_deep;

	QCheckBox *m_objects;
	QCheckBox *m_trees;
	QCheckBox *m_roads;
	QCheckBox *m_colorGrade;
	QCheckBox *m_renderedWater;
	QCheckBox *m_shaderWater;
	QCheckBox *m_depthTint;
	QComboBox *m_timeOfDay;
	QComboBox *m_area;
	QSpinBox *m_x0;
	QSpinBox *m_y0;
	QSpinBox *m_x1;
	QSpinBox *m_y1;
	QLabel *m_areaWarning;
	QComboBox *m_size;
	QComboBox *m_supersample;
	QLabel *m_renderInfo;
	QPushButton *m_renderBtn;

	WBQtHQCaptureParams m_rendered;
	bool m_renderDirty = false;
	bool m_loading = true;
};

}

extern "C" int WBQtHQPreview_Show(const char *tgaPath)
{
	WBQtHQPreviewDialog dlg(QString::fromLocal8Bit(tgaPath ? tgaPath : ""), WBQt_DialogParent());
	dlg.setWindowModality(Qt::ApplicationModal);
	return dlg.exec() == QDialog::Accepted ? 1 : 0;
}
