#pragma once

#include <bitset>

#include <QWidget>
#include <QColor>

class KeyboardWidget : public QWidget
{
    Q_OBJECT

public:
    KeyboardWidget(QWidget *parent = nullptr);
    ~KeyboardWidget() override;

    void setMuted(bool muted);
    void setPressedColor(const QColor &color);
    void setPressedKeys(const std::bitset<128> &pressed);
    const std::bitset<128> &getPressedKeys() const;

private:
    void paintEvent(QPaintEvent *paintEvent) override;

    const int WHITE_KEY_WIDTH = 6;
    const int BLACK_KEY_WIDTH = 5;
    const int OCTAVE_WIDTH = WHITE_KEY_WIDTH * 7; // 7 white keys per octave
    const int KEYBOARD_WIDTH = 10 * 7 * WHITE_KEY_WIDTH + 5 * WHITE_KEY_WIDTH + 1;

    std::bitset<128> pressed;
    QColor pressedColor{255, 0, 255};
    bool muted = false;

signals:
    void pressedChanged();
};
