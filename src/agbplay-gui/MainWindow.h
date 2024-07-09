#pragma once

#include <QMainWindow>
#include <QListView>
#include <QTableView>
#include <QTextEdit>

#include "SonglistWidget.h"
#include "RominfoWidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void Setup();

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
