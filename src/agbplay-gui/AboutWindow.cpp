#include "AboutWindow.h"

#include <fmt/core.h>

#include "Version.h"

static const std::string figletText = []() {
    std::string t;
    t += "<tt>";
    t += "             _         _             <br>";
    t += "   __ _ __ _| |__ _ __| |__ _ _  _   <br>";
    t += "  / _` / _` | '_ \\ '_ \\ / _` | || |  <br>";
    t += "  \\__,_\\__, |_.__/ .__/_\\__,_|\\_, |  <br>";
    t += "       |___/     |_|          |__/   <br>";
    t += "</tt>";
    std::string nt;
    for (auto c : t) {
        if (c == ' ')
            nt += "&nbsp;";
        else
            nt += c;
    }
    return nt;
}();

static const std::string aboutText = [](){
    std::string t;
    t += fmt::format("agbplay {}<br>", GIT_VERSION_STRING);
    t += "<br>";
    t += "The high quality GBA music player<br>";
    t += "<br>";
    t += fmt::format("Copyright (c) 2015-{} ipatix<br>", COPYRIGHT_YEAR);
    t += "and contributors<br>";
    t += "<br>";
    t += "News, Bug Reports, Updates on <a href=\"https://github.com/ipatix/agbplay\">Github</a>";
    return t;
}();


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
    figletLabel.setText(QString::fromStdString(figletText));
    figletLabel.setAlignment(Qt::AlignCenter);
    figletLabel.setAutoFillBackground(true);
    figletLabel.setFrameStyle(QFrame::Sunken | QFrame::Panel);
    figletLabel.setLineWidth(3);

    aboutInfoLabel.setText(QString::fromStdString(aboutText));
    aboutInfoLabel.setAlignment(Qt::AlignCenter);
    aboutInfoLabel.setOpenExternalLinks(true);

    connect(&okButton, &QAbstractButton::clicked, [this](bool){ accept(); });
}

AboutWindow::~AboutWindow()
{
}
