#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QHBoxLayout>

#include <cstdint>
#include <vector>

class SonglistWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SonglistWidget(const QString &titleString, QWidget *parent = nullptr);
    ~SonglistWidget() override;

    void AddSong(const std::string &name, uint16_t id);
    void RemoveSong();

private:
    QVBoxLayout layout{this};
    QListWidget listWidget{this};
    QLabel title{this};
};
