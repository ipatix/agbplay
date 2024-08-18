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
    void reset();

    QVBoxLayout layout{this};
    QHBoxLayout upperLayout;
    QHBoxLayout lowerLayout;

    QLabel titleLabel{this};
    QLabel bpmLabel{this};
    QLabel bpmFactorLabel{this};
    QLabel chnLabel{this};
    QLabel timeLabel{this};
    KeyboardWidget keyboardWidget{this};
    QLabel chordLabel{this};
};
