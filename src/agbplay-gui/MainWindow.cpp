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
    Setup();
}

MainWindow::~MainWindow()
{
}

void MainWindow::Setup()
{
    /* Status Bar */
    statusBar()->showMessage("HELLO");

    /* Menu Bar */
    QMenu *fileMenu = menuBar()->addMenu("File");
    QAction *fileOpenAction = fileMenu->addAction("Open");
    QAction *fileQuitAction = fileMenu->addAction("Quit");
    connect(fileQuitAction, &QAction::triggered, [this](bool) { close(); });
    fileQuitAction->setMenuRole(QAction::QuitRole);
    QMenu *helpMenu = menuBar()->addMenu("Help");
    QAction *helpAboutAction = helpMenu->addAction("About");
    connect(helpAboutAction, &QAction::triggered, [this](bool) {
        QMessageBox mbox(QMessageBox::Icon::Information, "TEST", "TEST 2", QMessageBox::Ok, this);
        mbox.exec();
    });
    helpAboutAction->setMenuRole(QAction::AboutRole);

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
