#include "SonglistWidget.h"

SonglistWidget::SonglistWidget(const QString &titleString, QWidget *parent)
    : QWidget(parent)
{
    layout.setContentsMargins(5, 5, 0, 0);
    layout.addWidget(&title);
    layout.addWidget(&listWidget);
    title.setText(titleString);
    listWidget.setUniformItemSizes(true);
}

SonglistWidget::~SonglistWidget()
{
}

void SonglistWidget::AddSong(const std::string &name, uint16_t id)
{
    QListWidgetItem *item = new QListWidgetItem();
    try {
        item->setCheckState(Qt::Checked);
        item->setText(QString::fromStdString(name)); // TODO make name a wstring to avoid character loss on Windows
        item->setData(Qt::UserRole, id);
        item->setToolTip("Click checkbox in order to mark song for bulk export");
    } catch (...) {
        delete item;
        throw;
    }
    listWidget.addItem(item);
}
