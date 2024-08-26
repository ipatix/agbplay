#pragma once

#include <QGroupBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpacerItem>

class RominfoWidget : public QGroupBox
{
    Q_OBJECT

public:
    explicit RominfoWidget(QWidget *parent = nullptr);
    ~RominfoWidget() override;

    QVBoxLayout layout{this};

    QLabel romNameLabel{"ROM Name", this};
    QLineEdit romNameLineEdit{"<none>", this};
    QLabel romCodeLabel{"ROM Code", this};
    QLineEdit romCodeLineEdit{"<none>", this};
    QLabel songTableLabel{"Songtable Offset", this};
    QLineEdit songTableLineEdit{"<none>", this};
    QLabel songCountLabel{"Number of Songs:", this};
    QLineEdit songCountLineEdit{"<none>", this};
    QGroupBox soundModeGroupBox{"Sound Mode:", this};

    QVBoxLayout soundModeGroupBoxLayout{&soundModeGroupBox};

    QLabel pcmVolLabel{"PCM Volume:", &soundModeGroupBox};
    QLabel pcmVolValLabel{"<none>", &soundModeGroupBox};
    QLabel pcmRevLabel{"Force Reverb:", &soundModeGroupBox};
    QLabel pcmRevValLabel{"<none>", &soundModeGroupBox};
    QLabel pcmFreqLabel{"PCM Freq:", &soundModeGroupBox};
    QLabel pcmFreqValLabel{"<none>", &soundModeGroupBox};
    QLabel pcmChnLabel{"PCM Max Chn:", &soundModeGroupBox};
    QLabel pcmChnValLabel{"<none>", &soundModeGroupBox};
    QLabel pcmDacLabel{"DAC Config:", &soundModeGroupBox};
    QLabel pcmDacValLabel{"<none>", &soundModeGroupBox};

    QWidget spacer{this};
};
