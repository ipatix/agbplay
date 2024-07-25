#include "StatusWidget.h"

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

void KeyboardWidget::paintEvent(QPaintEvent *paintEvent)
{
    (void)paintEvent;

    QPainter painter{this};

    // TODO use the actual state instead of dummy array
    static std::bitset<128> keysPressed;
    keysPressed[4] = true;
    keysPressed[7] = true;
    keysPressed[8] = true;

    static const std::array<uint8_t, 7> W_KEY_TABLE{ 0, 2, 4, 5, 7, 9, 11 };
    static const std::array<uint8_t, 5> B_KEY_TABLE{ 1, 3, 6, 8, 10 };

    static const QColor b = QColor(0, 0, 0);
    static const QColor w = QColor(255, 255, 255);
    static const QColor pressed = QColor(255, 0, 255);
    static const QColor midc = QColor(150, 150, 150);
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
            if (keysPressed[key])
                col = pressed;
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
            if (keysPressed[key])
                col = QColor(255, 0, 255);
            else
                col = QColor(20, 20, 20);
            painter.fillRect(keyRect, col);

            painter.fillRect(QRect(keyRect.topLeft(), keyRect.bottomLeft()), b);
            painter.fillRect(QRect(keyRect.bottomLeft(), keyRect.bottomRight()), b);
            painter.fillRect(QRect(keyRect.topRight(), keyRect.bottomRight()), b);
            painter.fillRect(QRect(keyRect.topLeft(), keyRect.topRight()), b);
        }
    }
}

SongWidget::SongWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(32);
}

SongWidget::~SongWidget()
{
}

void SongWidget::paintEvent(QPaintEvent *paintEvent)
{
    (void)paintEvent;

    QPainter painter{this};
    painter.fillRect(contentsRect(), QColor(255, 0, 255));
}

TrackWidget::TrackWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(32);

    QPalette pal;
    pal.setColor(QPalette::Window, QColor(80, 80, 80));
    setPalette(pal);
    setAutoFillBackground(true);

    trackNoLabel.setText("X");
    trackNoLabel.setAlignment(Qt::AlignCenter);
    layout.addWidget(&trackNoLabel, 0, 0);
    layout.addWidget(&muteButton, 1, 1);
    layout.addWidget(&soloButton, 1, 0);
    layout.addWidget(&keyboardWidget, 1, 2);
    layout.addWidget(&vuBarWidget, 0, 2);
    layout.setContentsMargins(0, 0, 0, 0);

    vuBarWidget.setLevel(0.9f, 1.0f);
}

TrackWidget::~TrackWidget()
{
}

StatusWidget::StatusWidget(QWidget *parent)
    : QWidget(parent)
{
    layout.addWidget(&songWidget);
    layout.setContentsMargins(0, 0, 0, 0);
    layout.setSpacing(0);

    for (size_t i = 0; i < trackWidgets.size(); i++) {
        trackWidgets.at(i).setParent(this);
        layout.addWidget(&trackWidgets.at(i));
    }

}

StatusWidget::~StatusWidget()
{
}
