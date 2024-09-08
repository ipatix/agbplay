#pragma once

#include "SongWidget.hpp"
#include "TrackWidget.hpp"

#include <array>
#include <bitset>
#include <QFont>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

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

signals:
    void audibilityChanged(size_t trackNo, bool audible, bool visualOnly);
};
