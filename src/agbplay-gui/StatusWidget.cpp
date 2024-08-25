#include "StatusWidget.h"

#include <QPainter>

#include <fmt/core.h>

#include "Types.h"

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

#define setFmtText(...) setText(QString::fromStdString(fmt::format(__VA_ARGS__)))

void StatusWidget::setVisualizerState(const MP2KVisualizerState &state)
{
    if (state.players.size() == 0)
        return;

    if (maxChannels < state.activeChannels)
        maxChannels = state.activeChannels;

    songWidget.chnLabel.setFmtText("{}/{} Chn", state.activeChannels, maxChannels);

    // FIXME probably slow as fuck with all these string conversions, but it's a start

    const auto &player = state.players.at(state.primaryPlayer);

    songWidget.bpmLabel.setFmtText("{} BPM", player.bpm);
    songWidget.bpmFactorLabel.setFmtText("(x {:.4})", player.bpmFactor);
    songWidget.timeLabel.setFmtText("{:02}:{:02}", player.time / 60, player.time % 60);

    size_t i = 0;
    for (; i < std::min(static_cast<size_t>(player.tracksUsed), trackWidgets.size()); i++) {
        const auto &trk = player.tracks.at(i);
        auto &widget = *trackWidgets.at(i);

        widget.setVisible(true);
        widget.posLabel.setFmtText("0x{:07X}", trk.trackPtr); // TODO patt/pend
        widget.restLabel.setText(QString::number(trk.delay));
        if (trk.prog == PROG_UNDEFINED)
            widget.instNoLabel.setText("-");
        else
            widget.instNoLabel.setText(QString::number(trk.prog));
        widget.volLabel.setText(QString::number(trk.vol));
        if (trk.pan < 0)
            widget.panLabel.setFmtText("L{}", -trk.pan);
        else if (trk.pan > 0)
            widget.panLabel.setFmtText("R{}", trk.pan);
        else
            widget.panLabel.setText("C");
        widget.modLabel.setText(QString::number(trk.mod));
        widget.pitchLabel.setText(QString::number(trk.pitch));
        widget.keyboardWidget.setPressedKeys(trk.activeNotes);
        widget.vuBarWidgetLeft.setLevel(trk.envLFloat * 3.0f, 1.0f);
        widget.vuBarWidgetRight.setLevel(trk.envRFloat * 3.0f, 1.0f);
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

void StatusWidget::resetMuteSolo()
{
    for (TrackWidget *trackWidget : trackWidgets) {
        trackWidget->setSolo(false, true);
        trackWidget->setMuted(false, true);
    }
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
