#include "SongWidget.h"

SongWidget::SongWidget(QWidget *parent)
    : QWidget(parent)
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
    titleLabel.setFixedSize(300, 16);
    titleLabel.setFont(titleFont);
    titleLabel.setText("No game loaded");
    titleLabel.setPalette(labelPal);
    titleLabel.setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    upperLayout.addWidget(&titleLabel, 0);

    upperLayout.addSpacing(10);

    bpmLabel.setFixedSize(50, 16);
    bpmLabel.setText("0 BPM");
    bpmLabel.setPalette(labelPal);
    bpmLabel.setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    upperLayout.addWidget(&bpmLabel, 0);

    upperLayout.addSpacing(10);

    chnLabel.setFixedSize(70, 16);
    chnLabel.setText("0/0 Chn");
    chnLabel.setPalette(labelPal);
    chnLabel.setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    upperLayout.addWidget(&chnLabel, 0);

    upperLayout.addSpacing(10);

    timeLabel.setFixedSize(30, 16);
    timeLabel.setText("00:00");
    timeLabel.setPalette(labelPal);
    timeLabel.setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    upperLayout.addWidget(&timeLabel, 0);

    QFont font;
    font.setPointSize(18);
    QPalette chordPal;
    chordPal.setColor(QPalette::WindowText, QColor("#37dcdc"));
    chordLabel.setFixedSize(128, 32);
    chordLabel.setText("C Maj 7");
    chordLabel.setPalette(labelPal);
    chordLabel.setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    chordLabel.setFont(font);
    chordLabel.setFrameStyle(QFrame::Panel | QFrame::Plain);
    lowerLayout.addWidget(&chordLabel, 0);

    lowerLayout.addSpacing(10);

    keyboardWidget.setFixedHeight(32);
    keyboardWidget.setPressedColor(QColor(0, 220, 220));
    lowerLayout.addWidget(&keyboardWidget, 0);

    layout.setSpacing(0);
    layout.setContentsMargins(0, 0, 0, 0);

    upperLayout.addStretch(1);
    lowerLayout.addStretch(1);
}

SongWidget::~SongWidget()
{
}

void SongWidget::setPressed(const std::bitset<128> &pressed)
{
    keyboardWidget.setPressedKeys(pressed);
}
