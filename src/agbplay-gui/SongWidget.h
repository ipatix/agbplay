#pragma once

#include <QWidget>
#include <QGridLayout>
#include <QLabel>

#include <bitset>

#include "KeyboardWidget.h"

class SongWidget : public QWidget
{
    Q_OBJECT

public:
    SongWidget(QWidget *parent = nullptr);
    ~SongWidget() override;

    void setPressed(const std::bitset<128> &pressed);

private:
    QGridLayout layout{this};

    QLabel titleLabel{this};
    QLabel bpmLabel{this};
    QLabel chnLabel{this};
    QLabel timeLabel{this};
    KeyboardWidget keyboardWidget{this};
    QLabel chordLabel{this};

    enum {
        COL_PADL,
        COL_TITLE,
        COL_SPACE1,
        COL_CHN,
        COL_SPACE2,
        COL_TIME,
        COL_SPACE3,
        COL_KEYBOARD,
        COL_SPACE4,
        COL_CHORD,
        COL_PADR,
    };
};
