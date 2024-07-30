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

enum {
    COL_PADL = 0,
    COL_LABEL,
    COL_SPACE1,
    COL_BUTTONS1,
    COL_BUTTONS2,
    COL_SPACE2,
    COL_INST,
    COL_SPACE3,
    COL_VOL,
    COL_SPACE4,
    COL_PAN,
    COL_SPACE5,
    COL_MOD,
    COL_SPACE6,
    COL_PITCH,
    COL_SPACE7,
    COL_KEYS,
    COL_PADR
};

TrackWidget::TrackWidget(size_t trackNo, QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(36);

    layout.setRowMinimumHeight(0, 4);
    layout.setRowStretch(0, 0);

    layout.setColumnStretch(COL_PADL, 1);

    /* Label */
    trackNoLabel.setFixedSize(24, 32);
    trackNoLabel.setPalette(labelUnmutedPalette);
    trackNoLabel.setAutoFillBackground(true);
    trackNoLabel.setText(QString::number(trackNo));
    trackNoLabel.setAlignment(Qt::AlignCenter);
    trackNoLabel.setLineWidth(1);
    trackNoLabel.setMidLineWidth(1);
    trackNoLabel.setFrameStyle(QFrame::Box | QFrame::Raised);
    layout.addWidget(&trackNoLabel, 1, COL_LABEL, 2, 1);

    /* Spacer */
    layout.setColumnStretch(COL_SPACE1, 0);
    layout.setColumnMinimumWidth(COL_SPACE1, 2);

    /* Buttons */
    muteButton.setFixedSize(16, 16);
    muteButton.setStyleSheet("QPushButton {background-color: #CC0000;};");
    muteButton.setText("M");
    layout.addWidget(&muteButton, 1, COL_BUTTONS1);

    soloButton.setFixedSize(16, 16);
    soloButton.setStyleSheet("QPushButton {background-color: #00CC00;};");
    soloButton.setText("S");
    layout.addWidget(&soloButton, 2, COL_BUTTONS1);

    harmonyButton.setFixedSize(16, 16);
    harmonyButton.setStyleSheet("QPushButton {background-color: #00AAAA;};");
    harmonyButton.setText("H");
    layout.addWidget(&harmonyButton, 1, COL_BUTTONS2);
    
    /* Spacer */
    layout.setColumnStretch(COL_SPACE2, 0);
    layout.setColumnMinimumWidth(COL_SPACE2, 2);

    /* Labels */
    posLabel.setFixedSize(64, 16);
    posLabel.setPalette(posPalette);
    posLabel.setFont(labelMonospaceFont);
    posLabel.setText("0x0000000");
    posLabel.setAlignment(Qt::AlignRight);
    layout.addWidget(&posLabel, 1, COL_INST, 1, 5);

    restLabel.setFixedSize(20, 16);
    restLabel.setPalette(restPalette);
    restLabel.setFont(labelFont);
    restLabel.setText("0");
    restLabel.setAlignment(Qt::AlignRight);
    layout.addWidget(&restLabel, 1, COL_MOD);

    voiceTypeLabel.setFixedSize(35, 16);
    voiceTypeLabel.setPalette(voiceTypePalette);
    voiceTypeLabel.setFont(labelFont);
    voiceTypeLabel.setText("NONE");
    voiceTypeLabel.setAlignment(Qt::AlignRight);
    layout.addWidget(&voiceTypeLabel, 1, COL_PITCH);

    instNoLabel.setFixedSize(20, 16);
    instNoLabel.setPalette(instNoPalette);
    instNoLabel.setFont(labelFont);
    instNoLabel.setText("0");
    instNoLabel.setAlignment(Qt::AlignRight);
    layout.addWidget(&instNoLabel, 2, COL_INST);

    layout.setColumnStretch(COL_SPACE3, 0);
    layout.setColumnMinimumWidth(COL_SPACE3, 2);

    volLabel.setFixedSize(20, 16);
    volLabel.setPalette(volPalette);
    volLabel.setFont(labelFont);
    volLabel.setText("0");
    volLabel.setAlignment(Qt::AlignRight);
    layout.addWidget(&volLabel, 2, COL_VOL);

    layout.setColumnStretch(COL_SPACE4, 0);
    layout.setColumnMinimumWidth(COL_SPACE4, 2);

    panLabel.setFixedSize(20, 16);
    panLabel.setPalette(panPalette);
    panLabel.setFont(labelFont);
    panLabel.setText("C");
    panLabel.setAlignment(Qt::AlignRight);
    layout.addWidget(&panLabel, 2, COL_PAN);

    layout.setColumnStretch(COL_SPACE5, 0);
    layout.setColumnMinimumWidth(COL_SPACE5, 2);

    modLabel.setFixedSize(20, 16);
    modLabel.setPalette(modPalette);
    modLabel.setFont(labelFont);
    modLabel.setText("0");
    modLabel.setAlignment(Qt::AlignRight);
    layout.addWidget(&modLabel, 2, COL_MOD);

    layout.setColumnStretch(COL_SPACE6, 0);
    layout.setColumnMinimumWidth(COL_SPACE6, 2);

    pitchLabel.setFixedSize(35, 16);
    pitchLabel.setPalette(pitchPalette);
    pitchLabel.setFont(labelFont);
    pitchLabel.setText("00000");
    pitchLabel.setAlignment(Qt::AlignRight);
    layout.addWidget(&pitchLabel, 2, COL_PITCH);

    /* Spacer */
    layout.setColumnStretch(COL_SPACE5, 0);
    layout.setColumnMinimumWidth(COL_SPACE7, 10);

    /* Keyboard and VU bars */
    layout.addLayout(&vuBarKeyboardLayout, 1, COL_KEYS, 2, 1);
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
    vuBarWidgetLeft.setFixedHeight(8);
    vuBarWidgetRight.setLevel(0.9f, 1.0f);
    vuBarWidgetRight.setFixedHeight(8);

    layout.setColumnStretch(COL_PADR, 1);
}

TrackWidget::~TrackWidget()
{
}

const QPalette TrackWidget::labelUnmutedPalette = []() {
    QPalette pal;
    pal.setColor(QPalette::Window, QColor("#173917"));
    pal.setColor(QPalette::WindowText, QColor(255, 255, 255));
    pal.setColor(QPalette::Light, QColor("#6ffd6f"));
    pal.setColor(QPalette::Mid, QColor("#4af44a"));
    pal.setColor(QPalette::Dark, QColor("#009200"));
    return pal;
}();

const QPalette TrackWidget::labelMutedPalette = []() {
    QPalette pal;
    pal.setColor(QPalette::Window, QColor("#391717"));
    pal.setColor(QPalette::WindowText, QColor(255, 255, 255));
    pal.setColor(QPalette::Light, QColor("#fd6f6f"));
    pal.setColor(QPalette::Mid, QColor("#f44a4a"));
    pal.setColor(QPalette::Dark, QColor("#920000"));
    return pal;
}();

const QPalette TrackWidget::posPalette = []() {
    QPalette pal;
    pal.setColor(QPalette::WindowText, QColor("#27b927"));
    return pal;
}();

const QPalette TrackWidget::restPalette = []() {
    QPalette pal;
    pal.setColor(QPalette::WindowText, QColor("#ff2121"));
    return pal;
}();

const QPalette TrackWidget::voiceTypePalette = []() {
    QPalette pal;
    pal.setColor(QPalette::WindowText, QColor("#e7c900"));
    return pal;
}();

const QPalette TrackWidget::instNoPalette = []() {
    QPalette pal;
    pal.setColor(QPalette::WindowText, QColor("#e762bc"));
    return pal;
}();

const QPalette TrackWidget::volPalette = []() {
    QPalette pal;
    pal.setColor(QPalette::WindowText, QColor("#2ac642"));
    return pal;
}();

const QPalette TrackWidget::panPalette = []() {
    QPalette pal;
    pal.setColor(QPalette::WindowText, QColor("#e67920"));
    return pal;
}();

const QPalette TrackWidget::modPalette = []() {
    QPalette pal;
    pal.setColor(QPalette::WindowText, QColor("#00d0db"));
    return pal;
}();

const QPalette TrackWidget::pitchPalette = []() {
    QPalette pal;
    pal.setColor(QPalette::WindowText, QColor("#b800e7"));
    return pal;
}();

const QFont TrackWidget::labelFont = []() {
    QFont font;
    font.setPointSize(8);
    return font;
}();

const QFont TrackWidget::labelMonospaceFont = []() {
    QFont font;
    font.setPointSize(8);
    font.setFixedPitch(true);
    return font;
}();

StatusWidget::StatusWidget(QWidget *parent)
    : QWidget(parent)
{
    layout.addWidget(&songWidget);
    layout.setContentsMargins(0, 0, 0, 0);
    layout.setSpacing(0);

    QPalette pal;
    pal.setColor(QPalette::Window, QColor(20, 20, 20));
    setPalette(pal);
    setAutoFillBackground(true);

    for (size_t i = 0; i < 16; i++) {
        trackWidgets.push_back(new TrackWidget(i, this));
        layout.addWidget(trackWidgets.at(i));
    }
}

StatusWidget::~StatusWidget()
{
}
