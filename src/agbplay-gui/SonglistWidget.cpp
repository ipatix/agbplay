#include "SonglistWidget.h"

SonglistWidget::SonglistWidget(const QString &titleString, bool editable, QWidget *parent)
    : QWidget(parent), editable(editable), playIcon(":/icons/playlist-play.ico"), stopIcon(":/icons/playlist-stop.ico")
{
    setMinimumWidth(150);
    if (editable)
        listWidget.setDragDropMode(QAbstractItemView::InternalMove);
    layout.setContentsMargins(5, 5, 0, 0);
    layout.addWidget(&title);
    layout.addWidget(&listWidget);
    title.setText(titleString);
    listWidget.setUniformItemSizes(true);
}

SonglistWidget::~SonglistWidget()
{
}

void SonglistWidget::Clear()
{
    selectedSong = 0;
    playing = false;
    listWidget.clear();
}

void SonglistWidget::AddSong(const std::string &name, uint16_t id)
{
    QListWidgetItem *item = new QListWidgetItem();
    try {
        if (listWidget.count() == 0) {
            selectedSong = 0;
            item->setIcon(playing ? playIcon : stopIcon);
        }

        //const auto flags = Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
        //if (editable)
        //    item->setFlags(flags | Qt::ItemIsDropEnabled);
        //else
        //    item->setFlags(flags);

        item->setCheckState(Qt::Checked);
        item->setText(QString::fromStdString(name)); // TODO make name a wstring to avoid character loss on Windows
        item->setData(Qt::UserRole, static_cast<unsigned int>(id));
        item->setToolTip("Click checkbox in order to mark song for bulk export");
    } catch (...) {
        delete item;
        throw;
    }
    listWidget.addItem(item);
}

void SonglistWidget::SetPlayState(bool playing)
{
    this->playing = playing;
    if (QListWidgetItem *item = listWidget.item(static_cast<int>(selectedSong)); item != nullptr)
        item->setIcon(playing ? playIcon : stopIcon);
}
