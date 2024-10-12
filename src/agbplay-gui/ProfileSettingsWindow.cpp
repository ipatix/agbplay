#include "ProfileSettingsWindow.hpp"

#include "ui_ProfileSettingsWindow.h"

ProfileSettingsWindow::ProfileSettingsWindow(QWidget *parent) : QDialog(parent), ui(new Ui::ProfileSettingsWindow)
{
    ui->setupUi(this);
}

ProfileSettingsWindow::~ProfileSettingsWindow()
{
    delete ui;
}
