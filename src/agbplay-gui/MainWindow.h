#pragma once

#include <QMainWindow>
#include <QListView>
#include <QTableView>
#include <QTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QProgressBar>

#include "SonglistWidget.h"
#include "RominfoWidget.h"
#include "VUMeterWidget.h"
#include "StatusWidget.h"

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
    QSplitter containerCentralSplitter{Qt::Horizontal, &containerCentral};

    /* .left container */
    QWidget containerLeft{&containerCentral};
    QVBoxLayout containerLeftLayout{&containerLeft};

    /* ..song widgets */
    SonglistWidget songlistWidget{"Songlist", &containerLeft};
    SonglistWidget playlistWidget{"Playlist", &containerLeft};

    /* .right container */
    QWidget containerRight{&containerCentral};
    QVBoxLayout containerRightLayout{&containerRight};
    QSplitter containerRightSplitter{Qt::Vertical, &containerRight};

    /* ..status-info + log container */
    QWidget containerStatusInfo{&containerRight};
    QHBoxLayout containerStatusInfoLayout{&containerStatusInfo};
    QSplitter containerStatusInfoSplitter{Qt::Horizontal, &containerStatusInfo};

    /* ...status widget */
    StatusWidget statusWidget{&containerStatusInfo};

    /* ...info widget */
    RominfoWidget infoWidget{&containerStatusInfo};

    /* ..log widget */
    QTextEdit logWidget{&containerRight};

    /* status bar */
    QProgressBar progressBar;
};
