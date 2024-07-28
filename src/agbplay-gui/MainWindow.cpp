#include "MainWindow.h"

#include <QStatusBar>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QToolBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    SetupMenuBar();
    SetupToolBar();
    SetupWidgets();
    SetupStatusBar();
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
    toolBar->addWidget(&stopButton);
    playButton.setIcon(QIcon(":/icons/playback-play.ico"));
    playButton.setFixedSize(32, 32);
    playButton.setIconSize(QSize(32, 32));
    toolBar->addWidget(&playButton);
    pauseButton.setIcon(QIcon(":/icons/playback-pause.ico"));
    pauseButton.setFixedSize(32, 32);
    pauseButton.setIconSize(QSize(32, 32));
    toolBar->addWidget(&pauseButton);
    toolBar->addSeparator();
    toolBar->addWidget(&vuMeter);

    /* test */
    vuMeter.SetLevel(0.8f, 0.9f, 1.0f, 1.1f);
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
    containerLeftLayout.addWidget(&playlistWidget);
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
    statusBar()->showMessage("HELLO");
    statusBar()->addPermanentWidget(&progressBar);
    progressBar.setMaximumHeight(16);
    progressBar.setMinimumHeight(16);
    progressBar.setMaximumWidth(128);
    progressBar.setMinimumWidth(128);
    progressBar.setRange(0, 100);
    progressBar.setValue(20);
    progressBar.hide(); // progress bar is only shown on demand
}
