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

TrackWidget::TrackWidget(size_t trackNo, QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(36);

    //setStyleSheet("QWidget {background-color: #202020};");
    QPalette pal;
    pal.setColor(QPalette::Window, QColor(0x20, 0x20, 0x20));
    setPalette(pal);
    setAutoFillBackground(true);

    trackNoLabel.setText(QString::number(trackNo));
    trackNoLabel.setAlignment(Qt::AlignCenter);
    trackNoLabel.setStyleSheet("QLabel {color: #FFFFFF; font-family: \"Monospace\";};");
    layout.addWidget(&separatorLine, 0, 0, 1, -1);
    separatorLine.setFrameStyle(QFrame::Raised | QFrame::HLine);
    separatorLine.setLineWidth(2);
    separatorLine.setMidLineWidth(2);
    layout.addWidget(&trackNoLabel, 1, 1);
    layout.addWidget(&muteButton, 2, 2);
    layout.addWidget(&soloButton, 2, 1);
    layout.addLayout(&vuBarKeyboardLayout, 1, 3, 2, 1);
    layout.setColumnStretch(0, 1);
    layout.setColumnStretch(4, 1);
    layout.setContentsMargins(0, 0, 0, 0);
    layout.setHorizontalSpacing(0);
    layout.setVerticalSpacing(0);

    vuBarKeyboardLayout.addWidget(&vuBarWidget);
    vuBarKeyboardLayout.addWidget(&keyboardWidget);
    vuBarKeyboardLayout.setContentsMargins(0, 0, 0, 0);
    vuBarKeyboardLayout.setSpacing(0);

    vuBarLayout.addWidget(&vuBarWidgetLeft);
    vuBarLayout.addWidget(&vuBarWidgetRight);
    vuBarLayout.setContentsMargins(0, 0, 0, 0);
    vuBarLayout.setSpacing(0);
    vuBarWidgetLeft.setLevel(0.9f, 1.0f);
    //vuBarWidgetLeft.setFrameStyle(QFrame::Sunken | QFrame::Panel);
    //vuBarWidgetLeft.setLineWidth(1);
    vuBarWidgetLeft.setFixedHeight(8);
    vuBarWidgetRight.setLevel(0.9f, 1.0f);
    //vuBarWidgetRight.setFrameStyle(QFrame::Sunken | QFrame::Panel);
    //vuBarWidgetRight.setLineWidth(1);
    vuBarWidgetRight.setFixedHeight(8);
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

    for (size_t i = 0; i < 16; i++) {
        trackWidgets.push_back(new TrackWidget(i, this));
        layout.addWidget(trackWidgets.at(i));
    }
}

StatusWidget::~StatusWidget()
{
}
