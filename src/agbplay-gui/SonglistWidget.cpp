#include "SonglistWidget.h"

#include <QEvent>
#include <QInputDialog>

SonglistWidget::SonglistWidget(const QString &titleString, bool editable, QWidget *parent)
    : QWidget(parent), editable(editable), playIcon(":/icons/playlist-play.ico"), stopIcon(":/icons/playlist-stop.ico")
{
    setMinimumWidth(150);
    if (editable)
        listWidget.setDragDropMode(QAbstractItemView::InternalMove);
    layout.setContentsMargins(5, 5, 0, 0);

    titleBarLayout.addWidget(&title);
    titleBarLayout.addWidget(&addRemoveButton);
    titleBarLayout.addWidget(&selectAllCheckBox);

    layout.addLayout(&titleBarLayout);
    layout.addWidget(&listWidget);

    title.setText(titleString);

    addRemoveButton.setFixedSize(24, 24);
    addRemoveButton.setIconSize(QSize(16, 16));
    if (editable) {
        addRemoveButton.setIcon(QIcon(":/icons/playlist-remove.ico"));
        addRemoveButton.setToolTip("Remove selected song from playlist");
    } else {
        addRemoveButton.setIcon(QIcon(":/icons/playlist-add.ico"));
        addRemoveButton.setToolTip("Add selected song to playlist");
    }

    selectAllCheckBox.setTristate(true);
    selectAllCheckBox.setToolTip("Select/deselect all songs for export");
    selectAllCheckBox.setFixedSize(16, 16);
    connect(&selectAllCheckBox, &QAbstractButton::clicked, [this](bool) { UpdateCheckedFromCheckBox(); });
    // Qt 6.7 only
    //connect(&selectAllCheckBox, &QCheckBox::checkStateChanged, [this](Qt::CheckState) { UpdateCheckedFromCheckBox(); });

    listWidget.setUniformItemSizes(true);
    listWidget.installEventFilter(this);
    listWidget.setContextMenuPolicy(Qt::ActionsContextMenu);
    if (editable) {
        QAction *action = listWidget.addAction("Remove from playlist");
        action->setIcon(QIcon(":/icons/playlist-remove.ico"));
        action->setShortcut(QKeySequence(Qt::Key_Delete));
        action->setShortcutContext(Qt::WidgetShortcut);
        connect(action, &QAction::triggered, [this](bool) { emit ContextMenuActionRemove(); });

        action = listWidget.addAction("Rename");
        action->setShortcut(QKeySequence(Qt::Key_F2));
        action->setShortcutContext(Qt::WidgetShortcut);
        connect(action, &QAction::triggered, [this](bool) { Rename(); });
    } else {
        QAction *action = listWidget.addAction("Add to playlist");
        action->setIcon(QIcon(":/icons/playlist-add.ico"));
        action->setShortcut(QKeySequence(Qt::Key_Insert));
        action->setShortcutContext(Qt::WidgetShortcut);
        connect(action, &QAction::triggered, [this](bool) { emit ContextMenuActionAdd(); });
    }
    connect(&listWidget, &QListWidget::itemChanged, [this](QListWidgetItem *) { UpdateCheckedFromItems(); });
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

    if (listWidget.count() == 1) {
        selectAllCheckBox.setCheckState(Qt::Checked);
    } else if (selectAllCheckBox.checkState() != Qt::Checked) {
        selectAllCheckBox.setCheckState(Qt::PartiallyChecked);
    }

    emit ContentChanged();
}

void SonglistWidget::RemoveSong()
{
    QList<QListWidgetItem *> items = listWidget.selectedItems();
    for (int i = 0; i < items.count(); i++) {
        QListWidgetItem *item = items.at(i);
        if (!item)
            continue;
        const int itemRow = listWidget.row(item);
        assert(itemRow >= 0 && itemRow < listWidget.count());
        if (itemRow < selectedSong)
            selectedSong--;
        delete item;
    }

    emit ContentChanged();
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

void SonglistWidget::UpdateCheckedFromItems()
{
    bool noneChecked = true;
    bool allChecked = true;

    for (int i = 0; i < listWidget.count(); i++) {
        QListWidgetItem *item = listWidget.item(i);
        if (!item)
            continue;

        if (item->checkState() == Qt::Checked)
            noneChecked = false;
        else
            allChecked = false;
    }

    if (noneChecked)
        selectAllCheckBox.setCheckState(Qt::Unchecked);
    else if (allChecked)
        selectAllCheckBox.setCheckState(Qt::Checked);
    else
        selectAllCheckBox.setCheckState(Qt::PartiallyChecked);
}

void SonglistWidget::UpdateCheckedFromCheckBox()
{
    Qt::CheckState checkState = selectAllCheckBox.checkState();
    if (checkState == Qt::PartiallyChecked) {
        /* Not sure how safe this is. This assumes that clicking changes
         * from unchecked -> partiallychecked -> checked -> unchecked.
         * If this were not the case, the checkbox may be stuck on. */
        checkState = Qt::Checked;
        selectAllCheckBox.setCheckState(checkState);
    }

    for (int i = 0; i < listWidget.count(); i++) {
        QListWidgetItem *item = listWidget.item(i);
        if (!item)
            continue;

        item->setCheckState(checkState);
    }
}

void SonglistWidget::Rename()
{
    QList<QListWidgetItem *> items = listWidget.selectedItems();
    if (items.count() == 0)
        return;

    QListWidgetItem *item = items.at(0);

    bool ok = false;
    QString text = QInputDialog::getText(this, "Rename song", "Song name:", QLineEdit::Normal, item->text(), &ok);
    if (!ok || text.isEmpty())
        return;

    item->setText(text);

    emit ContentChanged();
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

    emit ContentChanged();

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
