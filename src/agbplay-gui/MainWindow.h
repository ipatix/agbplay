#pragma once

#include <memory>

#include <QMainWindow>
#include <QListView>
#include <QTableView>
#include <QTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QProgressBar>
#include <QScrollArea>

#include "SonglistWidget.h"
#include "RominfoWidget.h"
#include "VUMeterWidget.h"
#include "StatusWidget.h"

class PlaybackEngine;
class ProfileManager;
class Profile;

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

    void Play();
    void Pause();
    void Stop();
    void LoadSong(const std::string &title, uint16_t id);

    void LoadGame();

    /* menu bar */
    QPushButton pauseButton{this};
    QPushButton playButton{this};
    QPushButton stopButton{this};
    QPushButton prevButton{this};
    QPushButton nextButton{this};
    VUMeterWidget vuMeter{128, 32, this};

    /* central container */
    QWidget containerCentral{this};
    QHBoxLayout containerCentralLayout{&containerCentral};
    QSplitter containerCentralSplitter{Qt::Horizontal, &containerCentral};

    /* .left container */
    QWidget containerLeft{&containerCentral};
    QVBoxLayout containerLeftLayout{&containerLeft};

    /* ..song widgets */
    SonglistWidget songlistWidget{"Songlist", false, &containerLeft};
    SonglistWidget playlistWidget{"Playlist", true, &containerLeft};

    /* .right container */
    QWidget containerRight{&containerCentral};
    QVBoxLayout containerRightLayout{&containerRight};
    QSplitter containerRightSplitter{Qt::Vertical, &containerRight};

    /* ..status-info + log container */
    QWidget containerStatusInfo{&containerRight};
    QHBoxLayout containerStatusInfoLayout{&containerStatusInfo};
    QSplitter containerStatusInfoSplitter{Qt::Horizontal, &containerStatusInfo};

    /* ...status widget */
    QScrollArea statusWidgetScrollArea{&containerStatusInfo};
    StatusWidget statusWidget{&statusWidgetScrollArea};

    /* ...info widget */
    RominfoWidget infoWidget{&containerStatusInfo};

    /* ..log widget */
    QTextEdit logWidget{&containerRight};

    /* status bar */
    QProgressBar progressBar;

    /* MP2K Objects */
    std::unique_ptr<ProfileManager> pm;
    // TODO replace with smart pointer, requires changes in ProfileManager
    Profile *profile = nullptr;
    std::unique_ptr<PlaybackEngine> playbackEngine;

    bool playing = false;
    bool playlistFocus = false;
};
