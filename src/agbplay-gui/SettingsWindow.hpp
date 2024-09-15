#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QDialog>

namespace Ui
{
    class SettingsWindow;
}

class QComboBox;
class QAbstractButton;
struct Settings;

class SettingsWindow : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget *parent, Settings &settings);
    ~SettingsWindow();

private:
    void playbackComboBoxActivated(int index);
    void exportComboBoxActivated(int index);
    void updateComboBoxRate(QComboBox *comboBox, int &index, const int indexActivated);
    void exportPushButtonPressed(bool);
    void buttonBoxButtonPressed(QAbstractButton *button);

    Ui::SettingsWindow *ui;
    Settings &settings;

    int exportComboBoxIndex = 0;
    int playbackComboBoxIndex = 0;
};

#endif    // SETTINGSWINDOW_H
