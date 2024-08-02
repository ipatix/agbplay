#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QIcon>

#include <cstdint>
#include <vector>
#include <memory>

class SonglistWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SonglistWidget(const QString &titleString, bool editable, QWidget *parent = nullptr);
    ~SonglistWidget() override;

    void Clear();
    void AddSong(const std::string &name, uint16_t id);
    void RemoveSong();
    void SetPlayState(bool playing);
    void SetEditable(bool editable);

private:
    QVBoxLayout layout{this};
    QListWidget listWidget{this};
    QLabel title{this};

    size_t selectedSong = 0;
    bool playing = false;
    const bool editable = false;

    const QIcon playIcon;
    const QIcon stopIcon;
};
