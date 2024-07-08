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
    QVBoxLayout *vbox = nullptr;
    QHBoxLayout *hbox = nullptr;
    QWidget *containerCentral = new QWidget(this);
    QWidget *containerLeft = new QWidget(containerCentral);
    QWidget *containerRight = new QWidget(containerCentral);
    QWidget *containerStatusInfo = new QWidget(containerRight);
    hbox = new QHBoxLayout(containerCentral);
    hbox->addWidget(containerLeft);
    hbox->addWidget(containerRight);
    hbox->setStretch(0, 1);
    hbox->setStretch(1, 5);
    setCentralWidget(containerCentral);

    /* 2. Create songlist and playlist. */
    vbox = new QVBoxLayout(containerLeft);
    listViewSonglist = new QListView(containerLeft);
    listViewPlaylist = new QListView(containerLeft);
    vbox->addWidget(listViewSonglist);
    vbox->addWidget(listViewPlaylist);

    /* 3. Create rom info and main status view. */
    hbox = new QHBoxLayout(containerStatusInfo);
    tableViewStatus = new QTableView(containerStatusInfo);
    tableViewInfo = new QTableView(containerStatusInfo);
    hbox->addWidget(tableViewStatus);
    hbox->addWidget(tableViewInfo);
    hbox->setStretch(0, 5);
    hbox->setStretch(1, 1);

    /* 4. Create log */
    vbox = new QVBoxLayout(containerRight);
    textEditLog = new QTextEdit(containerRight);
    vbox->addWidget(containerStatusInfo);
    vbox->addWidget(textEditLog);
    vbox->setStretch(0, 5);
    vbox->setStretch(1, 1);

    centralWidget()->show();
}
