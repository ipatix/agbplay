#include "KeyboardWidget.h"

#include <QPainter>

KeyboardWidget::KeyboardWidget(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(24);
    setFixedWidth(KEYBOARD_WIDTH);
}

KeyboardWidget::~KeyboardWidget()
{
}

void KeyboardWidget::setMuted(bool muted)
{
    this->muted = muted;
    update();
}

void KeyboardWidget::setPressedColor(const QColor &color)
{
    pressedColor = color;
    update();
}

void KeyboardWidget::setPressedKeys(const std::bitset<128> &pressed)
{
    if (this->pressed != pressed) {
        this->pressed = pressed;
        emit pressedChanged();
        update();
    }
}

const std::bitset<128> &KeyboardWidget::getPressedKeys() const
{
    return pressed;
}

void KeyboardWidget::paintEvent(QPaintEvent *paintEvent)
{
    (void)paintEvent;

    QPainter painter{this};

    static const std::array<uint8_t, 7> W_KEY_TABLE{ 0, 2, 4, 5, 7, 9, 11 };
    static const std::array<uint8_t, 5> B_KEY_TABLE{ 1, 3, 6, 8, 10 };

    static const QColor b = QColor(0, 0, 0);
    static const QColor bk = QColor(20, 20, 20);
    static const QColor w = QColor(255, 255, 255);
    static const QColor pressedMuted = QColor(100, 100, 100);
    static const QColor midc = QColor(160, 160, 160);
    static const QColor c = QColor(200, 200, 200);

    for (int octave = 0; octave < 11; octave++) {
        const QRect octaveRect(octave * OCTAVE_WIDTH, 0, OCTAVE_WIDTH, height());

        for (int wkey = 0; wkey < 7; wkey++) {
            if (octave == 10 && wkey == 5) {
                /* draw final line */
                const int x = octaveRect.left() + wkey * WHITE_KEY_WIDTH;
                painter.fillRect(QRect(QPoint(x, octaveRect.top()), QPoint(x, octaveRect.bottom())), QColor(Qt::black));
                break;
            }

            const int key = octave * 12 + W_KEY_TABLE[wkey];
            assert(key < 128);

            const QRect keyRect(
                QPoint(
                    octaveRect.left() + wkey * WHITE_KEY_WIDTH,
                    octaveRect.top()
                ),
                QSize(
                    WHITE_KEY_WIDTH,
                    octaveRect.height()
                )
            );

            QColor col;
            if (pressed[key])
                col = muted ? pressedMuted : pressedColor;
            else if (key == 60)
                col = midc;
            else if (wkey == 0)
                col = c;
            else
                col = w;
            painter.fillRect(keyRect, col);

            /* Draw key only with top, left, and bottom border lines. The right border line will
             * be implicitly drawn by the next key. */
            painter.fillRect(QRect(keyRect.topLeft(), keyRect.bottomLeft()), b);
            painter.fillRect(QRect(keyRect.bottomLeft(), keyRect.bottomRight()), b);
            painter.fillRect(QRect(keyRect.topLeft(), keyRect.topRight()), b);
        }

        for (int bkey = 0; bkey < 5; bkey++) {
            if (octave == 10 && bkey == 3)
                break;

            const int key = octave * 12 + B_KEY_TABLE[bkey];
            assert(key < 128);

            const int BLACK_L_HALF = BLACK_KEY_WIDTH / 2;

            int keyWOff = 1;
            if (bkey >= 2)
                keyWOff += 1;

            const QRect keyRect(
                QPoint(
                    octaveRect.left() + (bkey + keyWOff) * WHITE_KEY_WIDTH - BLACK_L_HALF,
                    octaveRect.top()
                ),
                QSize(
                    BLACK_KEY_WIDTH,
                    octaveRect.height() * 6 / 10
                )
            );

            QColor col;
            if (pressed[key])
                col = muted ? pressedMuted : pressedColor;
            else
                col = bk;
            painter.fillRect(keyRect, col);

            painter.fillRect(QRect(keyRect.topLeft(), keyRect.bottomLeft()), b);
            painter.fillRect(QRect(keyRect.bottomLeft(), keyRect.bottomRight()), b);
            painter.fillRect(QRect(keyRect.topRight(), keyRect.bottomRight()), b);
            painter.fillRect(QRect(keyRect.topLeft(), keyRect.topRight()), b);
        }
    }
}
