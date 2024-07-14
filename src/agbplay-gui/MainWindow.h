#pragma once

#include <QMainWindow>
#include <QListView>
#include <QTableView>
#include <QTextEdit>
#include <QPushButton>

#include "SonglistWidget.h"
#include "RominfoWidget.h"
#include "VUMeterWidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void SetupMenuBar();
    void SetupToolBar();
    void SetupWidgets();
    void SetupStatusBar();

    /* menu bar */
    QPushButton pauseButton{this};
    QPushButton playButton{this};
    QPushButton stopButton{this};
    VUMeterWidget vuMeter{128, 32, this};

    /* central container */
    QWidget containerCentral{this};
    QHBoxLayout containerCentralLayout{&containerCentral};

    /* .left container */
    QWidget containerLeft{&containerCentral};
    QVBoxLayout containerLeftLayout{&containerLeft};

    /* ..song widgets */
    SonglistWidget songlistWidget{"Songlist", &containerLeft};
    SonglistWidget playlistWidget{"Playlist", &containerLeft};

    /* .right container */
    QWidget containerRight{&containerCentral};
    QVBoxLayout containerRightLayout{&containerRight};

    /* ..status-info + log container */
    QWidget containerStatusInfo{&containerRight};
    QHBoxLayout containerStatusInfoLayout{&containerStatusInfo};

    /* ...status widget */
    QTextEdit statusWidget{&containerStatusInfo};

    /* ...info widget */
    RominfoWidget infoWidget{&containerStatusInfo};

    /* ..log widget */
    QTextEdit logWidget{&containerRight};
};
