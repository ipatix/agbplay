#ifndef PROFILESETTINGSWINDOW_H
#define PROFILESETTINGSWINDOW_H

#include <QDialog>
#include <memory>

struct Profile;
class ProfileManager;
class QAbstractButton;

namespace Ui
{
    class ProfileSettingsWindow;
}

class ProfileSettingsWindow : public QDialog
{
    Q_OBJECT

public:
    explicit ProfileSettingsWindow(QWidget *parent, ProfileManager &pm, std::shared_ptr<Profile> &profile);
    ~ProfileSettingsWindow();

private:
    void InitButtonBar();
    void InitTreeWidget();
    void InitProfileInfo();
    void InitSoundMode();
    void InitGameTables();
    void UpdateProfileList();
    void Apply();
    void buttonBoxButtonPressed(QAbstractButton *button);

    Ui::ProfileSettingsWindow *ui;
    ProfileManager &pm;
    std::shared_ptr<Profile> &profile;

    enum {
        COL_CODES = 0, COL_NAME, COL_AUTHOR, COL_STUDIO, COL_COUNT
    };

    enum {
        COL_PLT_TRACKS = 0, COL_PLT_PRIO, COL_PLT_COUNT
    };
};

#endif    // PROFILESETTINGSWINDOW_H
