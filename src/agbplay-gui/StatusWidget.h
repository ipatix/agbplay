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

#include "SongWidget.h"
#include "TrackWidget.h"

struct MP2KVisualizerState;

class ChordLabelWidget : public QFrame
{
    Q_OBJECT

public:
    ChordLabelWidget(QWidget *parent = nullptr);
    ~ChordLabelWidget() override;
};

class StatusWidget : public QFrame
{
    Q_OBJECT

public:
    StatusWidget(QWidget *parent = nullptr);
    ~StatusWidget() override;

    void setVisualizerState(const MP2KVisualizerState &state);
    void reset();
    void loadSongReset();

    QVBoxLayout layout{this};
    SongWidget songWidget{this};

private:
    void updateMuteOrSolo(bool visualOnly);
    void updateAnalyzer();

    QFrame hlineWidget{this};
    std::vector<TrackWidget *> trackWidgets;

    size_t maxChannels = 0;

signals:
    void audibilityChanged(size_t trackNo, bool audible, bool visualOnly);
};
