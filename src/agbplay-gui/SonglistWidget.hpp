#pragma once

#include <cstdint>
#include <memory>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <vector>

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

    bool eventFilter(QObject *object, QEvent *event) override;

signals:
    void ContextMenuActionRemove();
    void ContextMenuActionAdd();
    void ContentChanged();
    void PlayActionTriggered(int row);

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
