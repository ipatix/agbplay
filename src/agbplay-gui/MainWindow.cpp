#include "MainWindow.h"

#include <QStatusBar>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QToolBar>
#include <QFileDialog>

#include <fmt/core.h>

#include "SelectProfileDialog.h"

#include "ProfileManager.h"
#include "Rom.h"
#include "PlaybackEngine.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    SetupMenuBar();
    SetupToolBar();
    SetupWidgets();
    SetupStatusBar();

    pm = std::make_unique<ProfileManager>();
    pm->LoadProfiles();
    connect(&statusUpdateTimer, &QTimer::timeout, this, &MainWindow::StatusUpdate);
    statusUpdateTimer.setTimerType(Qt::PreciseTimer);
    statusUpdateTimer.setInterval(16);
    statusUpdateTimer.start();
}

MainWindow::~MainWindow()
{
}

void MainWindow::SetupMenuBar()
{
    /* File */
    QMenu *fileMenu = menuBar()->addMenu("File");

    QAction *fileOpenRom = fileMenu->addAction("Open ROM");
    fileOpenRom->setIcon(QIcon(":/icons/open-rom.ico"));
    connect(fileOpenRom, &QAction::triggered, [this](bool){ LoadGame(); });

    QAction *fileOpenGSF = fileMenu->addAction("Open GSF");
    fileOpenGSF->setIcon(QIcon(":/icons/open-gsf.ico"));

    fileMenu->addSeparator();

    QAction *exportAudio = fileMenu->addAction("Export Audio");
    exportAudio->setIcon(QIcon(":/icons/export-audio.ico"));

    QAction *exportMidi = fileMenu->addAction("Export MIDI");
    exportMidi->setIcon(QIcon(":/icons/export-midi.ico"));

    fileMenu->addSeparator();

    QAction *fileQuit = fileMenu->addAction("Quit");
    fileQuit->setIcon(QIcon(":/icons/exit.ico"));

    connect(fileQuit, &QAction::triggered, [this](bool) { close(); });
    fileQuit->setMenuRole(QAction::QuitRole);

    /* Edit */
    QMenu *editMenu = menuBar()->addMenu("Edit");
    QAction *editPreferences = editMenu->addAction("Global Preferences");
    editPreferences->setIcon(QIcon(":/icons/preferences.ico"));

    /* Profile */
    QMenu *profileMenu = menuBar()->addMenu("Profile");
    QAction *profileSelect = profileMenu->addAction("Load Profile");
    profileSelect->setIcon(QIcon(":/icons/profile-select.ico"));
    profileSelect->setEnabled(false);
    QAction *profileCreate = profileMenu->addAction("Create New Profile");
    profileCreate->setIcon(QIcon(":/icons/profile-create.ico"));
    profileCreate->setEnabled(false);
    QAction *profileDuplicate = profileMenu->addAction("Duplicate Current Profile");
    profileDuplicate->setIcon(QIcon(":/icons/profile-duplicate.ico"));
    profileDuplicate->setEnabled(false);
    QAction *profileDelete = profileMenu->addAction("Delete Current Profile");
    profileDelete->setIcon(QIcon(":/icons/profile-delete.ico"));
    profileDelete->setEnabled(false);
    profileMenu->addSeparator();
    QAction *profileSettings = profileMenu->addAction("Profile Settings");
    profileSettings->setIcon(QIcon(":/icons/profile-settings.ico"));
    profileSettings->setEnabled(false);
    QAction *profileMinigsfImport = profileMenu->addAction("Import GSF Playlist");
    profileMinigsfImport->setIcon(QIcon(":/icons/profile-import-minigsf.ico"));
    profileMinigsfImport->setEnabled(false);
    profileMenu->addSeparator();
    QAction *profileDirectory = profileMenu->addAction("Open User Profile Directory");
    profileDirectory->setIcon(QIcon(":/icons/profile-open-folder.ico"));

    /* Help */
    QMenu *helpMenu = menuBar()->addMenu("Help");
    QAction *helpSaveLog = helpMenu->addAction("Save Log");
    helpSaveLog->setIcon(QIcon(":/icons/save-log.ico"));
    helpMenu->addSeparator();
    QAction *helpAboutAction = helpMenu->addAction("About");
    helpAboutAction->setIcon(QIcon(":/icons/about.ico"));
    connect(helpAboutAction, &QAction::triggered, [this](bool) {
        QMessageBox mbox(QMessageBox::Icon::Information, "TEST", "TEST 2", QMessageBox::Ok, this);
        mbox.exec();
    });
    helpAboutAction->setMenuRole(QAction::AboutRole);
}

void MainWindow::SetupToolBar()
{
    QToolBar *toolBar = addToolBar("HAHAHAA");
    stopButton.setIcon(QIcon(":/icons/playback-stop.ico"));
    stopButton.setFixedSize(32, 32);
    stopButton.setIconSize(QSize(32, 32));
    stopButton.setCheckable(true);
    stopButton.setChecked(true);
    toolBar->addWidget(&stopButton);
    connect(&stopButton, &QAbstractButton::clicked, [this](bool) { Stop(); });

    playButton.setIcon(QIcon(":/icons/playback-play.ico"));
    playButton.setFixedSize(32, 32);
    playButton.setIconSize(QSize(32, 32));
    playButton.setCheckable(true);
    toolBar->addWidget(&playButton);
    connect(&playButton, &QAbstractButton::clicked, [this](bool) { Play(); });

    pauseButton.setIcon(QIcon(":/icons/playback-pause.ico"));
    pauseButton.setFixedSize(32, 32);
    pauseButton.setIconSize(QSize(32, 32));
    pauseButton.setCheckable(true);
    toolBar->addWidget(&pauseButton);
    connect(&pauseButton, &QAbstractButton::clicked, [this](bool) { Pause(); });

    prevButton.setIcon(QIcon(":/icons/playback-previous.ico"));
    prevButton.setFixedSize(32, 32);
    prevButton.setIconSize(QSize(32, 32));
    toolBar->addWidget(&prevButton);
    connect(&prevButton, &QAbstractButton::clicked, [this](bool) { AdvanceSong(-1); });

    nextButton.setIcon(QIcon(":/icons/playback-next.ico"));
    nextButton.setFixedSize(32, 32);
    nextButton.setIconSize(QSize(32, 32));
    toolBar->addWidget(&nextButton);
    connect(&nextButton, &QAbstractButton::clicked, [this](bool) { AdvanceSong(1); });

    toolBar->addSeparator();

    speedHalveButton.setIcon(QIcon(":/icons/speed-halve.ico"));
    speedHalveButton.setFixedSize(32, 32);
    speedHalveButton.setIconSize(QSize(32, 32));
    toolBar->addWidget(&speedHalveButton);
    connect(&speedHalveButton, &QAbstractButton::clicked, [this](bool) { SpeedHalve(); });

    speedDoubleButton.setIcon(QIcon(":/icons/speed-double.ico"));
    speedDoubleButton.setFixedSize(32, 32);
    speedDoubleButton.setIconSize(QSize(32, 32));
    toolBar->addWidget(&speedDoubleButton);
    connect(&speedDoubleButton, &QAbstractButton::clicked, [this](bool) { SpeedDouble(); });

    toolBar->addSeparator();

    toolBar->addWidget(&vuMeter);
}

void MainWindow::SetupWidgets()
{
    /* 1. Create containers. */
    containerCentralLayout.addWidget(&containerCentralSplitter);
    containerCentralLayout.setContentsMargins(0, 0, 0, 0);
    containerCentralSplitter.setChildrenCollapsible(false);
    containerCentralSplitter.addWidget(&containerLeft);
    containerCentralSplitter.addWidget(&containerRight);
    containerCentralSplitter.setStretchFactor(0, 1);
    containerCentralSplitter.setStretchFactor(1, 4);
    setCentralWidget(&containerCentral);

    /* 2. Create songlist and playlist. */
    containerLeftLayout.addWidget(&songlistWidget);
    connect(&songlistWidget.listWidget, &QAbstractItemView::doubleClicked, [this](const QModelIndex &index) {
            const QListWidgetItem *item = songlistWidget.listWidget.item(index.row());
            assert(item);
            if (!item)
                return;
            playlistFocus = false;
            LoadSong(item->text().toStdString(), static_cast<uint16_t>(item->data(Qt::UserRole).toUInt()));
            songlistWidget.SelectSong(index.row());
            Play();
    });
    containerLeftLayout.addWidget(&playlistWidget);
    connect(&playlistWidget.listWidget, &QAbstractItemView::doubleClicked, [this](const QModelIndex &index) {
            const QListWidgetItem *item = playlistWidget.listWidget.item(index.row());
            assert(item);
            if (!item)
                return;
            playlistFocus = true;
            LoadSong(item->text().toStdString(), static_cast<uint16_t>(item->data(Qt::UserRole).toUInt()));
            playlistWidget.SelectSong(index.row());
            Play();
    });
    containerLeftLayout.setContentsMargins(0, 0, 0, 0);

    /* 3. Create rom info and main status view. */
    containerStatusInfoLayout.addWidget(&containerStatusInfoSplitter);
    containerStatusInfoLayout.setContentsMargins(0, 0, 0, 0);
    containerStatusInfoSplitter.setChildrenCollapsible(false);
    containerStatusInfoSplitter.addWidget(&statusWidgetScrollArea);
    containerStatusInfoSplitter.addWidget(&infoWidget);
    containerStatusInfoSplitter.setStretchFactor(0, 5);
    containerStatusInfoSplitter.setStretchFactor(1, 1);
    statusWidgetScrollArea.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    statusWidgetScrollArea.setWidgetResizable(true);
    statusWidgetScrollArea.setWidget(&statusWidget);
    infoWidget.setMaximumWidth(250);

    /* 4. Create log */
    containerRightLayout.addWidget(&containerRightSplitter);
    containerRightSplitter.setChildrenCollapsible(false);
    containerRightSplitter.addWidget(&containerStatusInfo);
    containerRightSplitter.addWidget(&logWidget);
    containerRightSplitter.setStretchFactor(0, 5);
    containerRightSplitter.setStretchFactor(1, 1);
    containerRightLayout.setContentsMargins(0, 0, 0, 0);
    logWidget.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    logWidget.setFont(QFont("Monospace"));
    logWidget.setText("hello");

    centralWidget()->show();
}

void MainWindow::SetupStatusBar()
{
    statusBar()->showMessage("No game loaded");
    statusBar()->addPermanentWidget(&progressBar);
    progressBar.setMaximumHeight(16);
    progressBar.setMinimumHeight(16);
    progressBar.setMaximumWidth(128);
    progressBar.setMinimumWidth(128);
    progressBar.setRange(0, 100);
    progressBar.setValue(20);
    progressBar.hide(); // progress bar is only shown on demand
}

void MainWindow::Play()
{
    if (!playbackEngine) {
        playButton.setChecked(false);
        return;
    }

    playbackEngine->Play();
    playing = true;

    if (playlistFocus)
        playlistWidget.SetPlayState(true);
    else
        songlistWidget.SetPlayState(true);

    playButton.setChecked(true);
    stopButton.setChecked(false);
    pauseButton.setChecked(false);
}

void MainWindow::Pause()
{
    if (!playbackEngine) {
        pauseButton.setChecked(false);
        return;
    }

    playing = playbackEngine->Pause();

    playButton.setChecked(playing);
    stopButton.setChecked(false);
    pauseButton.setChecked(!playing);
}

void MainWindow::Stop()
{
    if (!playbackEngine) {
        stopButton.setChecked(false);
        return;
    }

    playbackEngine->Stop();
    playing = false;

    if (playlistFocus)
        playlistWidget.SetPlayState(false);
    else
        songlistWidget.SetPlayState(false);

    playButton.setChecked(false);
    stopButton.setChecked(true);
    pauseButton.setChecked(false);
}

void MainWindow::AdvanceSong(int indexDelta)
{
    if (!playbackEngine)
        return;

    SonglistWidget &listWidget = playlistFocus ? playlistWidget : songlistWidget;
    const int oldIndex = listWidget.GetSelectedSong();
    const int newIndex = oldIndex + indexDelta;

    QListWidgetItem *oldItem = listWidget.listWidget.item(oldIndex);
    QListWidgetItem *newItem = listWidget.listWidget.item(newIndex);

    if (oldItem == nullptr)
        return;

    if (newItem == nullptr) {
        Stop();
    } else {
        listWidget.SelectSong(newIndex);
        LoadSong(newItem->text().toStdString(), static_cast<uint16_t>(newItem->data(Qt::UserRole).toUInt()));
    }
}

void MainWindow::LoadSong(const std::string &title, uint16_t id)
{
    if (!playbackEngine)
        return;

    playbackEngine->LoadSong(id);
    statusWidget.songWidget.titleLabel.setText(QString::fromStdString(fmt::format("{} - {}", id, title)));

    if (playlistFocus) {
        songlistWidget.SetPlayState(false);
        playlistWidget.SetPlayState(playing);
    } else {
        songlistWidget.SetPlayState(playing);
        playlistWidget.SetPlayState(false);
    }

    if (playing)
        playbackEngine->Play();
}

void MainWindow::SpeedHalve()
{
    if (!playbackEngine)
        return;

    playbackEngine->SpeedHalve();
}

void MainWindow::SpeedDouble()
{
    if (!playbackEngine)
        return;

    playbackEngine->SpeedDouble();
}

void MainWindow::LoadGame()
{
    QFileDialog fileDialog(this);
    fileDialog.setFileMode(QFileDialog::ExistingFile);
    fileDialog.setNameFilter("GBA-ROMs (*.gba *.zip);;GSF-Lib (*.gsflib *.zip)");

    if (!fileDialog.exec())
        return;

    assert(fileDialog.selectedFiles().size() == 1);

    Stop();
    playbackEngine.reset();
    profile.reset();

    Rom::CreateInstance(fileDialog.selectedFiles().at(0).toStdWString());

    MP2KScanner scanner(Rom::Instance());
    auto scanResults = scanner.Scan();
    auto profileCandidates = pm->GetProfiles(Rom::Instance(), scanResults);
    // TODO add error handling via exceptions, this needs distinct exception types in libagbplay

    if (profileCandidates.size() > 1) {
        SelectProfileDialog profileDialog;
        for (const std::shared_ptr<Profile> &profile : profileCandidates)
            profileDialog.addToSelectionDialog(*profile);

        if (!profileDialog.exec())
            return;

        assert(profileDialog.selectedProfile() >= 0);

        profile = profileCandidates.at(profileDialog.selectedProfile());
    } else {
        profile = profileCandidates.at(0);
    }

    songlistWidget.Clear();
    for (uint16_t i = 0; i < profile->songTableInfoPlayback.count; i++) {
        auto songName = fmt::format("{:04}", i);
        songlistWidget.AddSong(songName, i);
    }

    playlistWidget.Clear();
    for (size_t i = 0; i < profile->playlist.size(); i++) {
        playlistWidget.AddSong(profile->playlist.at(i).name, profile->playlist.at(i).id);
    }

    infoWidget.romNameLineEdit.setText(QString::fromStdString(Rom::Instance().ReadString(0xA0, 12)));
    infoWidget.romCodeLineEdit.setText(QString::fromStdString(Rom::Instance().ReadString(0xAc, 4)));
    infoWidget.songTableLineEdit.setText(QString::fromStdString(fmt::format("0x{:X}", profile->songTableInfoPlayback.pos)));
    infoWidget.songCountLineEdit.setText(QString::number(profile->songTableInfoPlayback.count));

    playbackEngine = std::make_unique<PlaybackEngine>(*profile);
    visualizerState = std::make_unique<MP2KVisualizerState>();
}

void MainWindow::StatusUpdate()
{
    if (!playbackEngine || !visualizerState)
        return;

    playbackEngine->GetVisualizerState(*visualizerState);
    vuMeter.SetLevel(visualizerState->masterVolLeft, visualizerState->masterVolRight, 1.0f, 1.0f);
    statusWidget.setVisualizerState(*visualizerState);
}
