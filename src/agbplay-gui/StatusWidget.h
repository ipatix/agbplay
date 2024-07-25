#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>

#include <array>
#include <bitset>

#include "VUBarWidget.h"

class KeyboardWidget : public QWidget
{
    Q_OBJECT

public:
    KeyboardWidget(QWidget *parent = nullptr);
    ~KeyboardWidget() override;

private:
    void paintEvent(QPaintEvent *paintEvent) override;

    const int WHITE_KEY_WIDTH = 6;
    const int BLACK_KEY_WIDTH = 5;
    const int OCTAVE_WIDTH = WHITE_KEY_WIDTH * 7; // 7 white keys per octave
    const int KEYBOARD_WIDTH = 10 * 7 * WHITE_KEY_WIDTH + 5 * WHITE_KEY_WIDTH + 1;
};

class SongWidget : public QWidget
{
    Q_OBJECT

public:
    SongWidget(QWidget *parent = nullptr);
    ~SongWidget() override;

private:
    void paintEvent(QPaintEvent *paintEvent) override;
};

class TrackWidget : public QWidget
{
    Q_OBJECT

public:
    TrackWidget(QWidget *parent = nullptr);
    ~TrackWidget() override;

private:
    QGridLayout layout{this};

    QLabel trackNoLabel{this};
    QPushButton muteButton{this};
    QPushButton soloButton{this};
    KeyboardWidget keyboardWidget{this};

    QWidget vuBarWidget{this};
    QHBoxLayout vuBarLayout{&vuBarWidget};
    VUBarWidget vuBarWidgetLeft{VUBarWidget::Orientation::Left, true, -36.0f, 3.0f, &vuBarWidget};
    VUBarWidget vuBarWidgetRight{VUBarWidget::Orientation::Right, true, -36.0f, 3.0f, &vuBarWidget};
};

class StatusWidget : public QWidget
{
    Q_OBJECT

public:
    StatusWidget(QWidget *parent = nullptr);
    ~StatusWidget() override;

private:

    QVBoxLayout layout{this};

    SongWidget songWidget{this};
    // TODO use constant MAX_TRACKS instead of 16
    std::array<TrackWidget, 16> trackWidgets;
};
