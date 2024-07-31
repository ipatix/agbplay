#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QFont>
#include <QPalette>

#include <array>
#include <bitset>

#include "VUBarWidget.h"
#include "KeyboardWidget.h"

class ChordLabelWidget : public QFrame
{
    Q_OBJECT

public:
    ChordLabelWidget(QWidget *parent = nullptr);
    ~ChordLabelWidget() override;
};

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

private:
    void updateAnalyzer();

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

class StatusWidget : public QWidget
{
    Q_OBJECT

public:
    StatusWidget(QWidget *parent = nullptr);
    ~StatusWidget() override;

private:
    void updateMuteOrSolo();
    void updateAnalyzer();

    QVBoxLayout layout{this};

    SongWidget songWidget{this};
    QFrame hlineWidget{this};
    // TODO use constant MAX_TRACKS instead of 16
    std::vector<TrackWidget *> trackWidgets;
};
