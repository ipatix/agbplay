#pragma once

#include <QWidget>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

#include <bitset>

#include "KeyboardWidget.h"
#include "VUBarWidget.h"

class TrackWidget : public QWidget
{
    Q_OBJECT

public:
    TrackWidget(size_t trackNo, QWidget *parent = nullptr);
    ~TrackWidget() override;

    void setMuted(bool muted);
    bool isMuted() const;
    void setSolo(bool solo);
    bool isSolo() const;
    void setAnalyzer(bool analyzer);
    bool isAnalyzing() const;
    void setAudible(bool audible);
    const std::bitset<128> getPressed() const;
    void setPressed(const std::bitset<128> &pressed);

    QGridLayout layout{this};

    QLabel trackNoLabel{this};
    QPushButton muteButton{this};
    QPushButton soloButton{this};
    QPushButton analyzerButton{this};
    QLabel posLabel{this};
    QLabel restLabel{this};
    QLabel voiceTypeLabel{this};
    QLabel instNoLabel{this};
    QLabel volLabel{this};
    QLabel panLabel{this};
    QLabel modLabel{this};
    QLabel pitchLabel{this};
    KeyboardWidget keyboardWidget{this};

    QWidget vuBarWidget{this};
    QHBoxLayout vuBarLayout{&vuBarWidget};
    VUBarWidget vuBarWidgetLeft{VUBarWidget::Orientation::Left, false, -36.0f, 3.0f, &vuBarWidget};
    VUBarWidget vuBarWidgetRight{VUBarWidget::Orientation::Right, false, -36.0f, 3.0f, &vuBarWidget};
    QVBoxLayout vuBarKeyboardLayout;

private:
    void updateAnalyzer();

    static const QPalette labelUnmutedPalette;
    static const QPalette labelMutedPalette;

    static const QPalette mutedLabelPalette;
    static const QPalette posPalette;
    static const QPalette restPalette;
    static const QPalette voiceTypePalette;
    static const QPalette instNoPalette;
    static const QPalette volPalette;
    static const QPalette panPalette;
    static const QPalette modPalette;
    static const QPalette pitchPalette;
    static const QFont labelFont;
    static const QFont labelMonospaceFont;

    bool muted = false;
    bool solo = false;
    bool analyzer = false;

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
        COL_PADR,
    };

signals:
    void muteOrSoloChanged();
    void analyzerChanged();
};
