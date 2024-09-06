#include "StatusWidget.hpp"

#include <QPainter>

#include "Types.hpp"

ChordLabelWidget::ChordLabelWidget(QWidget *parent)
    : QFrame(parent)
{
}

ChordLabelWidget::~ChordLabelWidget()
{
}

StatusWidget::StatusWidget(QWidget *parent)
    : QFrame(parent)
{
    setFrameStyle(QFrame::Sunken | QFrame::Panel);
    setLineWidth(2);
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

    reset();

    layout.addStretch(1);
}

StatusWidget::~StatusWidget()
{
}

void StatusWidget::setVisualizerState(const MP2KVisualizerState &state)
{
    if (state.players.size() == 0)
        return;

    const auto &player = state.players.at(state.primaryPlayer);
    songWidget.setVisualizerState(player, state.activeChannels);

    size_t i = 0;
    for (; i < std::min(static_cast<size_t>(player.tracksUsed), trackWidgets.size()); i++) {
        const auto &trk = player.tracks.at(i);
        auto &widget = *trackWidgets.at(i);

        widget.setVisible(true);
        widget.setVisualizerState(trk);
    }

    for (; i < trackWidgets.size(); i++) {
        trackWidgets.at(i)->setVisible(false);
    }
}

void StatusWidget::reset()
{
    for (size_t i = 0; i < 16; i++)
        trackWidgets.at(i)->setVisible(false);
    songWidget.reset();
}

void StatusWidget::loadSongReset()
{
    for (TrackWidget *trackWidget : trackWidgets) {
        trackWidget->setSolo(false, true);
        trackWidget->setMuted(false, true);
    }

    songWidget.resetMaxChannels();
}

void StatusWidget::updateMuteOrSolo(bool visualOnly)
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
            emit audibilityChanged(trackWidget->getTrackNo(), trackWidget->isSolo(), visualOnly);
        } else {
            trackWidget->setAudible(!trackWidget->isMuted());
            emit audibilityChanged(trackWidget->getTrackNo(), !trackWidget->isMuted(), visualOnly);
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
