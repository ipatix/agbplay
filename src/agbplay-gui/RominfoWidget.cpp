#include "RominfoWidget.h"

RominfoWidget::RominfoWidget(QWidget *parent)
    : QGroupBox("ROM Info", parent)
{
    layout.addWidget(&romNameLabel);
    layout.addWidget(&romNameLineEdit);
    romNameLineEdit.setReadOnly(true);
    layout.addWidget(&romCodeLabel);
    layout.addWidget(&romCodeLineEdit);
    romCodeLineEdit.setReadOnly(true);
    layout.addWidget(&songTableLabel);
    layout.addWidget(&songTableLineEdit);
    songTableLineEdit.setReadOnly(true);
    layout.addWidget(&songCountLabel);
    layout.addWidget(&songCountLineEdit);
    songCountLineEdit.setReadOnly(true);
    layout.addWidget(&spacer);
    spacer.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    setMinimumWidth(130);
}

RominfoWidget::~RominfoWidget()
{
}
