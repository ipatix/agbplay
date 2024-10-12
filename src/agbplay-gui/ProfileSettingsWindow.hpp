#ifndef PROFILESETTINGSWINDOW_H
#define PROFILESETTINGSWINDOW_H

#include <QDialog>

namespace Ui
{
    class ProfileSettingsWindow;
}

class ProfileSettingsWindow : public QDialog
{
    Q_OBJECT

public:
    explicit ProfileSettingsWindow(QWidget *parent = nullptr);
    ~ProfileSettingsWindow();

private:
    Ui::ProfileSettingsWindow *ui;
};

#endif    // PROFILESETTINGSWINDOW_H
