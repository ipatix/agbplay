#include "AboutWindow.h"

static const QString figletText = []() {
    QString t;
    t += "<tt>";
    t += "             _         _             <br>";
    t += "   __ _ __ _| |__ _ __| |__ _ _  _   <br>";
    t += "  / _` / _` | '_ \\ '_ \\ / _` | || |  <br>";
    t += "  \\__,_\\__, |_.__/ .__/_\\__,_|\\_, |  <br>";
    t += "       |___/     |_|          |__/   <br>";
    t += "</tt>";
    QString nt;
    for (auto c : t) {
        if (c == ' ')
            nt += "&nbsp;";
        else
            nt += c;
    }
    return nt;
}();

static const QString aboutText =
    "agbplay v1.2.3<br>"
    "<br>"
    "The high quality GBA music player<br>"
    "<br>"
    "Copyright (c) 2015-2024 ipatix<br>"
    "and contributors<br>"
    "<br>"
    "News, Bug Reports, Updates on <a href=\"https://github.com/ipatix/agbplay\">Github</a>";


AboutWindow::AboutWindow(QWidget *parent)
    : QDialog(parent, Qt::Window)
{
    setFixedSize(600, 220);
    layout.addLayout(&topLayout);
    layout.addLayout(&botLayout);

    topLayout.addWidget(&figletLabel);
    topLayout.addWidget(&aboutInfoLabel);

    botLayout.addStretch(1);
    botLayout.addWidget(&okButton);
    botLayout.addStretch(1);

    QPalette pal;
    pal.setColor(QPalette::Window, QColor(20, 20, 20));
    pal.setColor(QPalette::WindowText, QColor(220, 220, 0));
    figletLabel.setPalette(pal);
    figletLabel.setText(figletText);
    figletLabel.setAlignment(Qt::AlignCenter);
    figletLabel.setAutoFillBackground(true);
    figletLabel.setFrameStyle(QFrame::Sunken | QFrame::Panel);
    figletLabel.setLineWidth(3);

    aboutInfoLabel.setText(aboutText);
    aboutInfoLabel.setAlignment(Qt::AlignCenter);

    connect(&okButton, &QAbstractButton::clicked, [this](bool){ accept(); });
}

AboutWindow::~AboutWindow()
{
}
