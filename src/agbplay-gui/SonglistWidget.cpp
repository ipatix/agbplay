#include "SonglistWidget.h"

#include <QEvent>

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

    listWidget.installEventFilter(this);
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

void SonglistWidget::SelectSong(int index)
{
    if (index == selectedSong)
        return;

    if (QListWidgetItem *item = listWidget.item(selectedSong); item != nullptr)
        item->setIcon(QIcon());

    if (QListWidgetItem *item = listWidget.item(index); item != nullptr) {
        item->setIcon(playing ? playIcon : stopIcon);
        listWidget.setCurrentRow(index);
    }

    selectedSong = index;
}

int SonglistWidget::GetSelectedSong() const
{
    return selectedSong;
}

bool SonglistWidget::eventFilter(QObject *object, QEvent *event)
{
    /* WARNING: I DO NOT KNOW WHAT I AM DOING.
     * Since there is apparently no signal for drag and dropping in the QListWidget,
     * we need a different way to detect when entries are moved, so that the currently
     * selected index can be updated and still remain valid.
     *
     * The only way I see is waiting for a ChildRemoved event
     * (I assume the floating item during drag&drop, which can then
     * trigger the validatio of the 'selectedSong' variable. */
    if (object != &listWidget)
        return false;

    if (event->type() != QEvent::ChildRemoved)
        return false;

    QListWidgetItem *item = listWidget.item(selectedSong);
    if (!item)
        return false;

    if (item->icon().isNull()) {
        /* If item is no longer the selected one, search for the first item with icon
         * and choose it as the new selected one. */
        for (int i = 0; i < listWidget.count(); i++) {
            QListWidgetItem *item = listWidget.item(i);
            if (!item)
                continue;
            if (!item->icon().isNull()) {
                selectedSong = i;
                return false;
            }
        }

        /* This case should not occur. At the same time it should not be fatal, but may
         * cause wrong icons to appear in the UI. */
        assert(false);
    }

    return false;
}
