#ifndef PROFILESETTINGSWINDOW_H
#define PROFILESETTINGSWINDOW_H

#include <QDialog>
#include <memory>

struct Profile;
class ProfileManager;

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
    void InitTreeWidget();
    void InitProfileInfo();
    void UpdateProfileList();

    Ui::ProfileSettingsWindow *ui;
    ProfileManager &pm;
    std::shared_ptr<Profile> &profile;

    enum {
        COL_CODES = 0, COL_NAME, COL_AUTHOR, COL_STUDIO, COL_COUNT
    };
};

#endif    // PROFILESETTINGSWINDOW_H
