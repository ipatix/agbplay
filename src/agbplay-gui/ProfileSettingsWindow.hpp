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
    explicit ProfileSettingsWindow(QWidget *parent, ProfileManager &pm, std::shared_ptr<Profile> &profile, const std::string &gameCode);
    ~ProfileSettingsWindow();

private:
    void InitButtonBar();
    void InitTreeWidget();
    void InitProfileInfo();
    void InitSoundMode();
    void InitEnhancements();
    void InitGameTables();
    void InitProfileAssignment();

    void MarkPending();
    void ClearPending();
    void UpdateProfileList();

    void Apply();

    Ui::ProfileSettingsWindow *ui;
    ProfileManager &pm;
    std::shared_ptr<Profile> &profile;
    std::string gameCode;
    bool pendingChanges = false;

    enum {
        COL_CODES = 0, COL_NAME, COL_AUTHOR, COL_STUDIO, COL_COUNT
    };

    enum {
        COL_PLT_TRACKS = 0, COL_PLT_PRIO, COL_PLT_COUNT
    };

    /* This string is only needed to keep track of changes in the magic byte
     * QLineEdit. It is used to check if a space was deleted at the cursor position in
     * order to also delete the previous character. */
    QString prevMagicBytes;
};

#endif    // PROFILESETTINGSWINDOW_H
