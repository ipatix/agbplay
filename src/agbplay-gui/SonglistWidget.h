#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QIcon>
#include <QCheckBox>
#include <QPushButton>

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
    void SelectSong(int index);
    int GetSelectedSong() const;

private:
    void UpdateCheckedFromItems();
    void UpdateCheckedFromCheckBox();
    void Rename();

    bool eventFilter(QObject *object, QEvent *event);

signals:
    void ContextMenuActionRemove();
    void ContextMenuActionAdd();
    void ContentChanged();

public:
    QVBoxLayout layout{this};
    QHBoxLayout titleBarLayout;
    QListWidget listWidget{this};
    QLabel title{this};
    QCheckBox selectAllCheckBox{this};
    QPushButton addRemoveButton{this};

private:
    int selectedSong = 0;
    bool playing = false;
    const bool editable = false;

    const QIcon playIcon;
    const QIcon stopIcon;
};
