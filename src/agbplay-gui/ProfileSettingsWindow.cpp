#include "ProfileSettingsWindow.hpp"

#include "ui_ProfileSettingsWindow.h"

#include <QAbstractButton>

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
    InitSoundMode();
}

void ProfileSettingsWindow::InitButtonBar()
{
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &ProfileSettingsWindow::buttonBoxButtonPressed);
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

    ui->treeWidget->setRootIsDecorated(false);
    ui->treeWidget->setSortingEnabled(true);
    ui->treeWidget->sortByColumn(COL_CODES, Qt::AscendingOrder);
}

void ProfileSettingsWindow::Apply()
{
    profile->name = ui->lineEditProfileName->text().toStdString();
    profile->author = ui->lineEditProfileAuthor->text().toStdString();
    profile->gameStudio = ui->lineEditGameStudio->text().toStdString();
    profile->description = ui->lineEditDescription->text().toStdString();
    profile->notes = ui->plainTextEditNotes->toPlainText().toStdString();
    profile->dirty = true;
}

void ProfileSettingsWindow::buttonBoxButtonPressed(QAbstractButton *button)
{
    const auto *okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    const auto *applyButton = ui->buttonBox->button(QDialogButtonBox::Apply);

    if (button != okButton && button != applyButton)
        return;

    Apply();
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

void ProfileSettingsWindow::InitSoundMode()
{
    /* volume */
    if (profile->mp2kSoundModeConfig.vol == MP2KSoundMode::VOL_AUTO) {
        printf("scanned vol: %d\n", profile->mp2kSoundModeScanned.vol);
        ui->spinBoxVol->setValue(profile->mp2kSoundModeScanned.vol);
        ui->spinBoxVol->setEnabled(false);
        ui->checkBoxVol->setCheckState(Qt::Unchecked);
    } else {
        ui->spinBoxVol->setValue(profile->mp2kSoundModeConfig.vol);
        ui->spinBoxVol->setEnabled(true);
        ui->checkBoxVol->setCheckState(Qt::Checked);
    }

    connect(ui->checkBoxVol, &QCheckBox::stateChanged, [this](int state){
        if (state == Qt::Checked) {
            ui->spinBoxVol->setEnabled(true);
        } else {
            ui->spinBoxVol->setValue(profile->mp2kSoundModeScanned.vol);
            ui->spinBoxVol->setEnabled(false);
        }
    });

    /* reverb */
    if (profile->mp2kSoundModeConfig.rev == MP2KSoundMode::REV_AUTO) {
        ui->spinBoxRev->setValue(profile->mp2kSoundModeScanned.rev);
        ui->spinBoxRev->setEnabled(false);
        ui->checkBoxRev->setCheckState(Qt::Unchecked);
    } else {
        ui->spinBoxRev->setValue(profile->mp2kSoundModeConfig.rev);
        ui->spinBoxRev->setEnabled(true);
        ui->checkBoxRev->setCheckState(Qt::Checked);
    }

    connect(ui->checkBoxRev, &QCheckBox::stateChanged, [this](int state){
        if (state == Qt::Checked) {
            ui->spinBoxRev->setEnabled(true);
        } else {
            ui->spinBoxRev->setValue(profile->mp2kSoundModeScanned.rev);
            ui->spinBoxRev->setEnabled(false);
        }
    });

    /* sample rate */
    static const std::array sampleRates{
        5734, 7884, 10512, 13379, 15768, 18157, 21024, 26758, 31536, 36314, 40137, 42048
    };

    for (unsigned int i = 0; i < sampleRates.size(); i++)
        ui->comboBoxFreq->addItem(QString::number(sampleRates.at(i)), i + 1);

    if (profile->mp2kSoundModeConfig.freq == MP2KSoundMode::FREQ_AUTO) {
        assert(profile->mp2kSoundModePlayback.freq > 0);
        ui->comboBoxFreq->setCurrentIndex(profile->mp2kSoundModeScanned.freq - 1);
        ui->comboBoxFreq->setEnabled(false);
        ui->checkBoxFreq->setCheckState(Qt::Unchecked);
    } else {
        assert(profile->mp2kSoundModeConfig.freq > 0);
        ui->comboBoxFreq->setCurrentIndex(profile->mp2kSoundModeConfig.freq - 1);
        ui->comboBoxFreq->setEnabled(true);
        ui->checkBoxFreq->setCheckState(Qt::Checked);
    }

    connect(ui->checkBoxFreq, &QCheckBox::stateChanged, [this](int state){
        if (state == Qt::Checked) {
            ui->comboBoxFreq->setEnabled(true);
        } else {
            ui->comboBoxFreq->setCurrentIndex(profile->mp2kSoundModeScanned.freq - 1);
            ui->comboBoxFreq->setEnabled(false);
        }
    });

    /* max channels */
    if (profile->mp2kSoundModeConfig.maxChannels == MP2KSoundMode::CHN_AUTO) {
        ui->spinBoxChn->setValue(profile->mp2kSoundModeScanned.maxChannels);
        ui->spinBoxChn->setEnabled(false);
        ui->checkBoxChn->setCheckState(Qt::Unchecked);
    } else {
        ui->spinBoxChn->setValue(profile->mp2kSoundModeConfig.maxChannels);
        ui->spinBoxChn->setEnabled(true);
        ui->checkBoxChn->setCheckState(Qt::Checked);
    }

    connect(ui->checkBoxChn, &QCheckBox::stateChanged, [this](int state){
        if (state == Qt::Checked) {
            ui->spinBoxChn->setEnabled(true);
        } else {
            ui->spinBoxChn->setValue(profile->mp2kSoundModeScanned.maxChannels);
            ui->spinBoxChn->setEnabled(false);
        }
    });

    /* DAC config */
    ui->comboBoxDac->addItem("32 kHz @ 9 bit", 8);
    ui->comboBoxDac->addItem("65 kHz @ 8 bit", 9);
    ui->comboBoxDac->addItem("131 kHz @ 7 bit", 10);
    ui->comboBoxDac->addItem("262 kHz @ 6 bit", 11);

    if (profile->mp2kSoundModeConfig.dacConfig == MP2KSoundMode::DAC_AUTO) {
        ui->comboBoxDac->setCurrentIndex(profile->mp2kSoundModeScanned.dacConfig % 4);
        ui->comboBoxDac->setEnabled(false);
        ui->checkBoxDac->setCheckState(Qt::Unchecked);
    } else {
        ui->comboBoxDac->setCurrentIndex(profile->mp2kSoundModeConfig.dacConfig % 4);
        ui->comboBoxDac->setEnabled(true);
        ui->checkBoxDac->setCheckState(Qt::Checked);
    }

    connect(ui->checkBoxDac, &QCheckBox::stateChanged, [this](int state){
        if (state == Qt::Checked) {
            ui->comboBoxDac->setEnabled(true);
        } else {
            ui->comboBoxDac->setCurrentIndex(profile->mp2kSoundModeScanned.dacConfig % 4);
            ui->comboBoxDac->setEnabled(false);
        }
    });
}

ProfileSettingsWindow::~ProfileSettingsWindow()
{
    delete ui;
}

void ProfileSettingsWindow::UpdateProfileList()
{
}
