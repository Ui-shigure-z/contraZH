// WBQtToast.cpp -- see WBQtToast.h. Mirrors CToastDialog: topmost tool windows stacked
// down the left edge of the screen from y=100, message + optional OK, auto-dismiss timer,
// tooltip.wav. Toasts self-remove from the stack on close (like the MFC vector); existing
// toasts keep their spot (no reflow), matching the MFC behavior.
#include "WBQtToast.h"

#include <QApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QMouseEvent>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include <qt_windows.h>
#include <mmsystem.h>

namespace
{
	const int kToastX = 10;
	const int kToastBaseY = 100;
	const int kToastSpacing = 8;
}

class WBQtToastWidget : public QFrame
{
public:
	WBQtToastWidget(const QString &text, int durationMs, bool showOkButton)
		: QFrame(NULL, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
	{
		setAttribute(Qt::WA_DeleteOnClose, true);
		setAttribute(Qt::WA_ShowWithoutActivating, true);
		setFrameShape(QFrame::StyledPanel);
		setFrameShadow(QFrame::Raised);

		QVBoxLayout *root = new QVBoxLayout(this);
		QLabel *label = new QLabel(text, this);
		root->addWidget(label);
		if (showOkButton)
		{
			QHBoxLayout *buttons = new QHBoxLayout();
			buttons->addStretch(1);
			QPushButton *okButton = new QPushButton("OK", this);
			okButton->setAutoDefault(false);
			buttons->addWidget(okButton);
			root->addLayout(buttons);
			connect(okButton, SIGNAL(clicked()), this, SLOT(close()));
		}

		// Stack below the toasts already showing (== CToastDialog's offset walk).
		int y = kToastBaseY;
		for (int i = 0; i < s_toasts.size(); i++)
		{
			y += s_toasts[i]->frameGeometry().height() + kToastSpacing;
		}
		s_toasts.append(this);

		adjustSize();
		move(kToastX, y);
		show();

		QTimer::singleShot(durationMs > 0 ? durationMs : 3000, this, SLOT(close()));
		::PlaySoundA("data\\editor\\audio\\tooltip.wav", NULL, SND_FILENAME | SND_ASYNC);
	}

	virtual ~WBQtToastWidget()
	{
		s_toasts.removeAll(this);
	}

protected:
	virtual void mousePressEvent(QMouseEvent *event)
	{
		Q_UNUSED(event);
		close();
	}

private:
	static QList<WBQtToastWidget *> s_toasts;
};

QList<WBQtToastWidget *> WBQtToastWidget::s_toasts;

extern "C" int WBQtToast_Show(const char *text, int durationMs, int showOkButton)
{
	if (qApp == NULL)
	{
		return 0;	// pre-Qt startup -- the caller falls back to the MFC toast
	}
	new WBQtToastWidget(QString::fromLocal8Bit((text != NULL) ? text : ""), durationMs,
		showOkButton != 0);
	return 1;
}
