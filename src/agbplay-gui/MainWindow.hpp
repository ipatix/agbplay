#pragma once

#include "RominfoWidget.hpp"
#include "SonglistWidget.hpp"
#include "StatusWidget.hpp"
#include "VUMeterWidget.hpp"

#include <atomic>
#include <filesystem>
#include <memory>
#include <QListView>
#include <QMainWindow>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QTableView>
#include <QTextEdit>
#include <QTimer>

class PlaybackEngine;
class ProfileManager;
struct Settings;
struct Profile;
namespace std
{
    class thread;
}
struct MP2KVisualizerState;

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
    void AdvanceSong(int indexDelta);
    void JumpSong();
    void JumpSong(int index);
    void LoadSong(const std::string &title, uint16_t id);
    void SpeedHalve();
    void SpeedDouble();

    void PlaylistAdd();
    void PlaylistRemove();
    void SonglistSwitchSong(int row);
    void PlaylistSwitchSong(int row);

    void ProfileImportGsfPlaylist(const std::filesystem::path &gameFilePath = "");
    bool AskGameCodeAndUpdateProfile();

    void LoadGame();
    bool CloseGame();
    void ExportAudio(bool benchmarkOnly, bool separateTracks, bool quick);
    void ExportStillInProgress();
    void SaveLog();
    void SaveProfile();
    void DiscardProfile();
    int AskSaveProfile();

    void MBoxInfo(const std::string &title, const std::string &msg);
    void MBoxError(const std::string &title, const std::string &msg);

    void UpdateSoundMode();
    void UpdateMute(size_t trackNo, bool audible, bool visualOnly);
    void StatusUpdate();
    static void LogCallback(const std::string &msg, void *void_this);
    Q_INVOKABLE void LogAppend(std::string msg);

    void closeEvent(QCloseEvent *event) override;

    /* menu bar */
    QAction *exportSongsAction = nullptr;
    QAction *exportStemsAction = nullptr;
    QAction *quickExportSongAction = nullptr;
    QAction *quickExportStemsAction = nullptr;
    QAction *benchmarkSelectedAction = nullptr;
    QAction *saveProfileAction = nullptr;
    QAction *profileSettings = nullptr;
    QAction *profileMinigsfImport = nullptr;

    /* tool bar */
    QPushButton saveButton{this};
    QPushButton pauseButton{this};
    QPushButton playButton{this};
    QPushButton stopButton{this};
    QPushButton prevButton{this};
    QPushButton nextButton{this};
    QPushButton speedHalveButton{this};
    QPushButton speedDoubleButton{this};
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
    QTimer statusUpdateTimer{this};
    std::unique_ptr<ProfileManager> pm;
    std::unique_ptr<Settings> settings;
    std::shared_ptr<Profile> profile;
    std::unique_ptr<PlaybackEngine> playbackEngine;
    std::unique_ptr<MP2KVisualizerState> visualizerState;

    bool playing = false;
    bool playlistFocus = false;

    /* File Export */
    std::unique_ptr<std::thread> exportThread;
    std::atomic<bool> exportBusy = false;

signals:
    /* Use a signal instead of QMetaObject::invokeMethod for pre Qt 6.7 compatibility. */
    void LogMessageReceived(const std::string &msg);
};
