#include "SonglistWidget.h"

SonglistWidget::SonglistWidget(QWidget *parent)
    : QWidget(parent)
{
    layout.setContentsMargins(5, 5, 0, 0);
    layout.addWidget(&title);
    layout.addWidget(&songList);
    title.setText("AAAA");
}

SonglistWidget::~SonglistWidget()
{
}
