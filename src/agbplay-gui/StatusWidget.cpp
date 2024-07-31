#include "StatusWidget.h"

#include <QPainter>

ChordLabelWidget::ChordLabelWidget(QWidget *parent)
    : QFrame(parent)
{
}

ChordLabelWidget::~ChordLabelWidget()
{
}

StatusWidget::StatusWidget(QWidget *parent)
    : QWidget(parent)
{
    layout.setContentsMargins(0, 0, 0, 0);
    layout.setSpacing(0);

    layout.addSpacing(2);

    layout.addWidget(&songWidget);

    QPalette pal;
    pal.setColor(QPalette::Window, QColor(20, 20, 20));
    setPalette(pal);
    setAutoFillBackground(true);

    layout.addSpacing(2);

    QPalette linePal;
    linePal.setColor(QPalette::WindowText, QColor(10, 240, 10));
    hlineWidget.setPalette(linePal);
    hlineWidget.setFrameStyle(QFrame::HLine | QFrame::Plain);
    hlineWidget.setLineWidth(0);
    hlineWidget.setMidLineWidth(1);
    layout.addWidget(&hlineWidget);

    for (size_t i = 0; i < 16; i++) {
        trackWidgets.push_back(new TrackWidget(i, this));
        layout.addWidget(trackWidgets.at(i));
        connect(trackWidgets.at(i), &TrackWidget::muteOrSoloChanged, this, &StatusWidget::updateMuteOrSolo);
        connect(trackWidgets.at(i), &TrackWidget::analyzerChanged, this, &StatusWidget::updateAnalyzer);
    }

    layout.addStretch(1);
}

StatusWidget::~StatusWidget()
{
}

void StatusWidget::updateMuteOrSolo()
{
    bool isSoloActive = false;
    for (const TrackWidget *trackWidget : trackWidgets) {
        if (!trackWidget->isVisible())
            continue;

        if (trackWidget->isSolo()) {
            isSoloActive = true;
            break;
        }
    }

    for (TrackWidget *trackWidget : trackWidgets) {
        if (!trackWidget->isVisible())
            continue;

        if (isSoloActive) {
            trackWidget->setAudible(trackWidget->isSolo());
        } else {
            trackWidget->setAudible(!trackWidget->isMuted());
        }
    }
}

void StatusWidget::updateAnalyzer()
{
    std::bitset<128> pressed;

    for (const TrackWidget *trackWidget : trackWidgets) {
        if (!trackWidget->isVisible())
            continue;

        if (trackWidget->isAnalyzing())
            pressed |= trackWidget->getPressed();
    }

    songWidget.setPressed(pressed);
}
