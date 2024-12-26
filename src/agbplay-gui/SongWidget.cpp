#include "SongWidget.hpp"

#include "Types.hpp"

#include <fmt/core.h>

SongWidget::SongWidget(QWidget *parent) : QWidget(parent)
{
    setFixedHeight(16 + 2 + 32);

    layout.addLayout(&upperLayout);
    layout.addSpacing(2);
    layout.addLayout(&lowerLayout);

    upperLayout.addStretch(1);
    lowerLayout.addStretch(1);

    QPalette labelPal;
    labelPal.setColor(QPalette::WindowText, QColor(255, 255, 255));

    QFont titleFont;
    titleFont.setUnderline(true);
    titleLabel.setFixedSize(320, 16);
    titleLabel.setFont(titleFont);
    titleLabel.setPalette(labelPal);
    titleLabel.setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    upperLayout.addWidget(&titleLabel, 0);

    upperLayout.addSpacing(10);

    bpmLabel.setFixedSize(50, 16);
    bpmLabel.setPalette(labelPal);
    bpmLabel.setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    upperLayout.addWidget(&bpmLabel, 0);

    upperLayout.addSpacing(5);

    bpmFactorLabel.setFixedSize(60, 16);
    bpmFactorLabel.setPalette(labelPal);
    bpmFactorLabel.setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    upperLayout.addWidget(&bpmFactorLabel, 0);

    upperLayout.addSpacing(10);

    chnLabel.setFixedSize(70, 16);
    chnLabel.setPalette(labelPal);
    chnLabel.setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    upperLayout.addWidget(&chnLabel, 0);

    upperLayout.addSpacing(30);

    timeLabel.setFixedSize(30, 16);
    timeLabel.setPalette(labelPal);
    timeLabel.setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    upperLayout.addWidget(&timeLabel, 0);

    QFont font;
    font.setPointSize(18);
    QPalette chordPal;
    chordPal.setColor(QPalette::WindowText, QColor("#37dcdc"));
    chordLabel.setFixedSize(128, 32);
    chordLabel.setPalette(labelPal);
    chordLabel.setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    chordLabel.setFont(font);
    chordLabel.setFrameStyle(QFrame::Panel | QFrame::Plain);
    lowerLayout.addWidget(&chordLabel, 0);

    lowerLayout.addSpacing(10);

    keyboardWidget.setFixedHeight(32);
    keyboardWidget.setPressedColor(QColor(255, 150, 0));
    lowerLayout.addWidget(&keyboardWidget, 0);

    layout.setSpacing(0);
    layout.setContentsMargins(0, 0, 0, 0);

    upperLayout.addStretch(1);
    lowerLayout.addStretch(1);

    reset();
}

SongWidget::~SongWidget()
{
}

#define setFmtText(...) setText(QString::fromStdString(fmt::format(__VA_ARGS__)))

void SongWidget::setVisualizerState(const MP2KVisualizerStatePlayer &state, size_t activeChannels)
{
    if (oldBpm != state.bpm) {
        oldBpm = state.bpm;
        bpmLabel.setFmtText("{} BPM", state.bpm);
    }

    if (oldBpmFactor != state.bpmFactor) {
        oldBpmFactor = state.bpmFactor;
        bpmFactorLabel.setFmtText("(x {:.4})", state.bpmFactor);
    }

    if (oldTime != state.time) {
        oldTime = state.time;
        timeLabel.setFmtText("{:02}:{:02}", state.time / 60, state.time % 60);
    }

    if (oldActiveChannels != activeChannels) {
        oldActiveChannels = activeChannels;

        if (maxChannels < activeChannels)
            maxChannels = activeChannels;

        chnLabel.setFmtText("{}/{} Chn", activeChannels, maxChannels);
    }
}

#undef setFmtText

void SongWidget::setPressed(const std::bitset<128> &pressed)
{
    keyboardWidget.setPressedKeys(pressed);
}

void SongWidget::reset()
{
    titleLabel.setText("No game loaded");
    bpmLabel.setText("150 BPM");
    bpmFactorLabel.setText("(x 1)");
    chnLabel.setText("0/0 Chn");
    timeLabel.setText("00:00");
    chordLabel.setText("---");
}

void SongWidget::resetMaxChannels()
{
    maxChannels = 0;
}
