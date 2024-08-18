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

    QVBoxLayout layout{this};
    SongWidget songWidget{this};

private:
    void updateMuteOrSolo();
    void updateAnalyzer();

    QFrame hlineWidget{this};
    // TODO use constant MAX_TRACKS instead of 16
    std::vector<TrackWidget *> trackWidgets;

    size_t maxChannels = 0;
};
