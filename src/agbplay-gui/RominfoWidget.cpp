#include "RominfoWidget.hpp"

RominfoWidget::RominfoWidget(QWidget *parent) : QGroupBox("ROM Info", parent)
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
    layout.addWidget(&soundModeGroupBox);
    layout.addWidget(&spacer);
    spacer.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    soundModeGroupBoxLayout.addWidget(&pcmVolLabel);
    soundModeGroupBoxLayout.addWidget(&pcmVolValLabel);
    soundModeGroupBoxLayout.addWidget(&pcmRevLabel);
    soundModeGroupBoxLayout.addWidget(&pcmRevValLabel);
    soundModeGroupBoxLayout.addWidget(&pcmFreqLabel);
    soundModeGroupBoxLayout.addWidget(&pcmFreqValLabel);
    soundModeGroupBoxLayout.addWidget(&pcmChnLabel);
    soundModeGroupBoxLayout.addWidget(&pcmChnValLabel);
    soundModeGroupBoxLayout.addWidget(&pcmDacLabel);
    soundModeGroupBoxLayout.addWidget(&pcmDacValLabel);

    pcmVolValLabel.setAlignment(Qt::AlignRight);
    // pcmVolValLabel.setReadOnly(true);
    pcmRevValLabel.setAlignment(Qt::AlignRight);
    // pcmRevValLabel.setReadOnly(true);
    pcmFreqValLabel.setAlignment(Qt::AlignRight);
    // pcmFreqValLabel.setReadOnly(true);
    pcmChnValLabel.setAlignment(Qt::AlignRight);
    // pcmChnValLabel.setReadOnly(true);
    pcmDacValLabel.setAlignment(Qt::AlignRight);
    // pcmDacValLabel.setReadOnly(true);

    setMinimumWidth(130);
}

RominfoWidget::~RominfoWidget()
{
}
