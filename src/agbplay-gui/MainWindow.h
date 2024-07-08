#pragma once

#include <QMainWindow>
#include <QListView>
#include <QTableView>
#include <QTextEdit>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void Setup();

    QListView *listViewSonglist = nullptr;
    QListView *listViewPlaylist = nullptr;
    QTableView *tableViewStatus = nullptr;
    QTableView *tableViewInfo = nullptr;
    QTextEdit *textEditLog = nullptr;
};
