#pragma once

#include <QWidget>
#include <QGridLayout>
#include <QLabel>

#include <bitset>

#include "KeyboardWidget.hpp"

struct MP2KVisualizerStatePlayer;

class SongWidget : public QWidget
{
    Q_OBJECT

public:
    SongWidget(QWidget *parent = nullptr);
    ~SongWidget() override;

    void setVisualizerState(const MP2KVisualizerStatePlayer &state, size_t activeChannels);
    void setPressed(const std::bitset<128> &pressed);
    void reset();
    void resetMaxChannels();

    QVBoxLayout layout{this};
    QHBoxLayout upperLayout;
    QHBoxLayout lowerLayout;

    QLabel titleLabel{this};

private:
    QLabel bpmLabel{this};
    QLabel bpmFactorLabel{this};
    QLabel chnLabel{this};
    QLabel timeLabel{this};
    KeyboardWidget keyboardWidget{this};
    QLabel chordLabel{this};

    uint16_t oldBpm = 0;
    float oldBpmFactor = 1.0f;
    size_t oldTime = 0;
    size_t oldActiveChannels = 0;
    size_t maxChannels = 0;
};
