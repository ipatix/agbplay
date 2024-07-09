#pragma once

#include <QWidget>
#include <QListView>
#include <QLabel>
#include <QHBoxLayout>

class SonglistWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SonglistWidget(QWidget *parent = nullptr);
    ~SonglistWidget() override;

private:
    QVBoxLayout layout{this};
    QListView songList{this};
    QLabel title{this};
};
