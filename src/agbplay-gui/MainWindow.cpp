#include "MainWindow.h"

#include <QStatusBar>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QTextEdit>

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
}

void MainWindow::SetupWidgets()
{
    /* 1. Create containers. */
    containerCentralLayout.addWidget(&containerLeft);
    containerCentralLayout.addWidget(&containerRight);
    containerCentralLayout.setStretch(0, 1);
    containerCentralLayout.setStretch(1, 5);
    containerCentralLayout.setContentsMargins(0, 0, 0, 0);
    setCentralWidget(&containerCentral);

    /* 2. Create songlist and playlist. */
    containerLeftLayout.addWidget(&songlistWidget);
    containerLeftLayout.addWidget(&playlistWidget);
    containerLeftLayout.setContentsMargins(0, 0, 0, 0);

    /* 3. Create rom info and main status view. */
    containerStatusInfoLayout.addWidget(&statusWidget);
    containerStatusInfoLayout.addWidget(&infoWidget);
    containerStatusInfoLayout.setStretch(0, 5);
    containerStatusInfoLayout.setStretch(1, 1);
    containerStatusInfoLayout.setContentsMargins(0, 0, 0, 0);

    /* 4. Create log */
    containerRightLayout.addWidget(&containerStatusInfo);
    containerRightLayout.addWidget(&logWidget);
    containerRightLayout.setStretch(0, 5);
    containerRightLayout.setStretch(1, 1);
    containerRightLayout.setContentsMargins(0, 0, 0, 0);
    logWidget.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    logWidget.setFont(QFont("Monospace"));
    logWidget.setText("hello");

    centralWidget()->show();
}

void MainWindow::SetupStatusBar()
{
    statusBar()->showMessage("HELLO");
}
