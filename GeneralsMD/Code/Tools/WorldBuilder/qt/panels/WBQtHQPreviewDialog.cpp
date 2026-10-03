// WBQtHQPreviewDialog.cpp -- shows the HQ map preview and its shading controls before the tga
// is written. See WBQtHQPreviewBridge.h.
#include "WBQtHQPreviewBridge.h"

#include <QColorDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

#include <vector>

// Modal-dialog parent (active modal if nested, else main window). WBQtBridge.cpp.
QWidget *WBQt_DialogParent(void);

namespace
{

const int kDisplayScale = 2;

class WBQtHQPreviewDialog : public QDialog
{
public:
	WBQtHQPreviewDialog(const QString &tgaPath, QWidget *parent)
		: QDialog(parent)
	{
		setWindowTitle(tr("HQ Map Preview"));
		const int size = WBQtHQPreview_Size();
		m_pixels.resize(size*size*4);

		m_image = new QLabel(this);
		m_image->setFixedSize(size*kDisplayScale, size*kDisplayScale);
		m_image->setFrameShape(QFrame::Box);

		QGridLayout *grid = new QGridLayout();
		m_relief = addSlider(grid, 0, tr("Relief"), 0, 200, true,
			tr("Strength of the hillshade that lights slopes from the north-west."));
		m_elevation = addSlider(grid, 1, tr("Elevation"), 0, 100, true,
			tr("How much brighter high ground is than low ground."));
		m_falloff = addSlider(grid, 2, tr("Water depth"), 5, 200, false,
			tr("Depth in world units at which water reaches most of its deep colour."));
		m_shallowBtn = addColorButton(grid, 3, tr("Shallow water"), m_shallow);
		m_deepBtn = addColorButton(grid, 4, tr("Deep water"), m_deep);

		QPushButton *resetBtn = new QPushButton(tr("Defaults"), this);
		connect(resetBtn, &QPushButton::clicked, this, [this]()
		{
			WBQtHQPreviewParams params;
			WBQtHQPreview_GetDefaults(&params);
			load(params);
		});

		QLabel *pathLabel = new QLabel(tgaPath, this);
		pathLabel->setWordWrap(true);
		pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

		QVBoxLayout *controls = new QVBoxLayout();
		controls->addLayout(grid);
		controls->addWidget(resetBtn, 0, Qt::AlignLeft);
		controls->addStretch(1);
		controls->addWidget(pathLabel);

		QHBoxLayout *body = new QHBoxLayout();
		body->addWidget(m_image, 0, Qt::AlignTop);
		body->addLayout(controls, 1);

		QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
		connect(buttons, &QDialogButtonBox::accepted, this, [this]()
		{
			WBQtHQPreviewParams params = current();
			if (WBQtHQPreview_Save(&params) != 0)
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

		WBQtHQPreviewParams params;
		WBQtHQPreview_GetLast(&params);
		load(params);
	}

private:
	QSlider *addSlider(QGridLayout *grid, int row, const QString &label, int lo, int hi, bool percent, const QString &help)
	{
		QLabel *name = new QLabel(label, this);
		QSlider *slider = new QSlider(Qt::Horizontal, this);
		QLabel *value = new QLabel(this);
		slider->setRange(lo, hi);
		value->setText(percent ? QString("%1%").arg(lo) : QString::number(lo));
		slider->setMinimumWidth(160);
		value->setMinimumWidth(40);
		name->setToolTip(help);
		slider->setToolTip(help);
		grid->addWidget(name, row, 0);
		grid->addWidget(slider, row, 1);
		grid->addWidget(value, row, 2);
		connect(slider, &QSlider::valueChanged, this, [this, value, percent](int v)
		{
			value->setText(percent ? QString("%1%").arg(v) : QString::number(v));
			refresh();
		});
		return slider;
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

	static void paintSwatch(QPushButton *button, const QColor &color)
	{
		button->setStyleSheet(QString("background-color: %1; border: 1px solid #555;").arg(color.name()));
	}

	void load(const WBQtHQPreviewParams &params)
	{
		m_loading = true;
		m_relief->setValue((int)(params.relief*100.0f + 0.5f));
		m_elevation->setValue((int)(params.elevation*100.0f + 0.5f));
		m_falloff->setValue((int)(params.waterFalloff + 0.5f));
		m_shallow = QColor(params.shallow[0], params.shallow[1], params.shallow[2]);
		m_deep = QColor(params.deep[0], params.deep[1], params.deep[2]);
		paintSwatch(m_shallowBtn, m_shallow);
		paintSwatch(m_deepBtn, m_deep);
		m_loading = false;
		refresh();
	}

	WBQtHQPreviewParams current() const
	{
		WBQtHQPreviewParams params;
		params.relief = m_relief->value() / 100.0f;
		params.elevation = m_elevation->value() / 100.0f;
		params.waterFalloff = (float)m_falloff->value();
		params.shallow[0] = m_shallow.red();
		params.shallow[1] = m_shallow.green();
		params.shallow[2] = m_shallow.blue();
		params.deep[0] = m_deep.red();
		params.deep[1] = m_deep.green();
		params.deep[2] = m_deep.blue();
		return params;
	}

	void refresh()
	{
		if (m_loading)
		{
			return;
		}
		const WBQtHQPreviewParams params = current();
		WBQtHQPreview_Compose(&params, &m_pixels[0]);
		const int size = WBQtHQPreview_Size();
		QImage image(&m_pixels[0], size, size, size*4, QImage::Format_RGB32);
		m_image->setPixmap(QPixmap::fromImage(image.scaled(size*kDisplayScale, size*kDisplayScale,
			Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
	}

	std::vector<unsigned char> m_pixels;
	QLabel *m_image;
	QSlider *m_relief;
	QSlider *m_elevation;
	QSlider *m_falloff;
	QPushButton *m_shallowBtn;
	QPushButton *m_deepBtn;
	QColor m_shallow;
	QColor m_deep;
	bool m_loading = true;
};

}

extern "C" int WBQtHQPreview_Show(const char *tgaPath)
{
	WBQtHQPreviewDialog dlg(QString::fromLocal8Bit(tgaPath ? tgaPath : ""), WBQt_DialogParent());
	dlg.setWindowModality(Qt::ApplicationModal);
	return dlg.exec() == QDialog::Accepted ? 1 : 0;
}
