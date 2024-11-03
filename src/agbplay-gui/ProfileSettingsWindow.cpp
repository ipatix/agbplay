#include "ProfileSettingsWindow.hpp"

#include "ui_ProfileSettingsWindow.h"

#include "Profile.hpp"
#include "ProfileManager.hpp"

static QColor profileColCur = QColor::fromHsv(210, 70, 255);
static QColor profileColAvail = QColor::fromHsv(120, 70, 255);
static QColor profileColUnavail = QColor::fromHsv(0, 70, 255);

ProfileSettingsWindow::ProfileSettingsWindow(QWidget *parent, ProfileManager &pm, std::shared_ptr<Profile> &profile)
    : QDialog(parent), ui(new Ui::ProfileSettingsWindow), pm(pm), profile(profile)
{
    ui->setupUi(this);
    InitTreeWidget();
    InitProfileInfo();
}

void ProfileSettingsWindow::InitTreeWidget()
{
    QStringList hlist;
    ui->treeWidget->setColumnCount(COL_COUNT);
    ui->treeWidget->setHeaderLabels(QStringList{"Game Code", "Name", "Author", "Game Studio"});
    ui->treeWidget->setColumnWidth(COL_CODES, 90);

    for (std::shared_ptr<Profile> &profileToInsert : pm.GetAllProfiles()) {
        QTreeWidgetItem *profileItem = new QTreeWidgetItem(ui->treeWidget, QTreeWidgetItem::Type);

        QStringList slist;
        for (const std::string &s : profileToInsert->gameMatch.gameCodes)
            slist.append(QString::fromStdString(s));
        profileItem->setText(COL_CODES, slist.join(", "));
        profileItem->setText(COL_NAME, QString::fromStdString(profileToInsert->name));
        profileItem->setText(COL_AUTHOR, QString::fromStdString(profileToInsert->author));
        profileItem->setText(COL_STUDIO, QString::fromStdString(profileToInsert->gameStudio));

        if (profileToInsert == profile)
            profileItem->setBackground(COL_CODES, profileColCur);
        else
            profileItem->setBackground(COL_CODES, profileColUnavail);
    }

    ui->treeWidget->setSortingEnabled(true);
    ui->treeWidget->sortByColumn(COL_CODES, Qt::AscendingOrder);
}

void ProfileSettingsWindow::InitProfileInfo()
{
    ui->lineEditProfileName->setText(QString::fromStdString(profile->name));
    ui->lineEditProfileAuthor->setText(QString::fromStdString(profile->author));
    ui->lineEditGameStudio->setText(QString::fromStdString(profile->gameStudio));
    ui->lineEditDescription->setText(QString::fromStdString(profile->description));
    ui->plainTextEditNotes->clear();
    ui->plainTextEditNotes->appendPlainText(QString::fromStdString(profile->notes));
}

ProfileSettingsWindow::~ProfileSettingsWindow()
{
    delete ui;
}

void ProfileSettingsWindow::UpdateProfileList()
{
}
