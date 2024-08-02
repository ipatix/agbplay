#include "SongWidget.h"

SongWidget::SongWidget(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(32);

    layout.setColumnStretch(COL_PADL, 1);
    layout.setColumnStretch(COL_SPACE1, 0);
    layout.setColumnMinimumWidth(COL_SPACE1, 10);
    layout.setColumnStretch(COL_SPACE2, 0);
    layout.setColumnMinimumWidth(COL_SPACE2, 10);
    layout.setColumnStretch(COL_SPACE3, 0);
    layout.setColumnMinimumWidth(COL_SPACE3, 2);
    layout.setColumnStretch(COL_SPACE4, 0);
    layout.setColumnMinimumWidth(COL_SPACE4, 2);
    layout.setColumnStretch(COL_PADR, 1);

    QPalette labelPal;
    labelPal.setColor(QPalette::WindowText, QColor(255, 255, 255));

    QFont titleFont;
    titleFont.setUnderline(true);
    titleLabel.setFixedSize(170, 16);
    titleLabel.setFont(titleFont);
    titleLabel.setText("0000 - Test Title AAA");
    titleLabel.setPalette(labelPal);
    titleLabel.setAlignment(Qt::AlignCenter);
    layout.addWidget(&titleLabel, 0, COL_TITLE, 1, 5);

    bpmLabel.setFixedSize(50, 16);
    bpmLabel.setText("0 BPM");
    bpmLabel.setPalette(labelPal);
    bpmLabel.setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout.addWidget(&bpmLabel, 1, COL_TITLE);

    chnLabel.setFixedSize(70, 16);
    chnLabel.setText("100/100 Chn");
    chnLabel.setPalette(labelPal);
    chnLabel.setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout.addWidget(&chnLabel, 1, COL_CHN);

    timeLabel.setFixedSize(30, 16);
    timeLabel.setText("00:00");
    timeLabel.setPalette(labelPal);
    timeLabel.setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout.addWidget(&timeLabel, 1, COL_TIME);

    keyboardWidget.setFixedHeight(32);
    keyboardWidget.setPressedColor(QColor(0, 220, 220));
    layout.addWidget(&keyboardWidget, 0, COL_KEYBOARD, 2, 1);

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
    layout.addWidget(&chordLabel, 0, COL_CHORD, 2, 1);

    layout.setHorizontalSpacing(0);
    layout.setVerticalSpacing(0);
    layout.setContentsMargins(0, 0, 0, 0);
}

SongWidget::~SongWidget()
{
}

void SongWidget::setPressed(const std::bitset<128> &pressed)
{
    keyboardWidget.setPressedKeys(pressed);
}
