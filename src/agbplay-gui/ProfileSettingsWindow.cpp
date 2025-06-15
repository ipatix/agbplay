#include "ProfileSettingsWindow.hpp"

#include "ui_ProfileSettingsWindow.h"

#include <QAbstractButton>
#include <QFileDialog>
#include <QMessageBox>

#include "Profile.hpp"
#include "ProfileManager.hpp"

static QColor profileColCur = QColor::fromHsv(210, 70, 255);
static QColor profileColAvail = QColor::fromHsv(120, 70, 255);
static QColor profileColUnavail = QColor::fromHsv(0, 70, 255);

ProfileSettingsWindow::ProfileSettingsWindow(QWidget *parent, ProfileManager &pm, std::shared_ptr<Profile> &profile, const std::string &gameCode)
    : QDialog(parent), ui(new Ui::ProfileSettingsWindow), pm(pm), profile(profile), gameCode(gameCode)
{
    ui->setupUi(this);
    InitButtonBar();
    InitTreeWidget();
    InitProfileInfo();
    InitSoundMode();
    InitEnhancements();
    InitGameTables();
    InitProfileAssignment();

    ClearPending();
}

ProfileSettingsWindow::~ProfileSettingsWindow()
{
    delete ui;
}

void ProfileSettingsWindow::InitButtonBar()
{
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(ui->buttonBox, &QDialogButtonBox::clicked, [this](QAbstractButton *button){
        const auto *okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
        const auto *applyButton = ui->buttonBox->button(QDialogButtonBox::Apply);

        if (button != okButton && button != applyButton)
            return;

        Apply();
    });
}

void ProfileSettingsWindow::InitTreeWidget()
{
    /* tree widget itself */
    auto addProfileToTreeWidget = [this](const Profile &profileToInsert) {
        QTreeWidgetItem *profileItem = new QTreeWidgetItem(ui->treeWidget, QTreeWidgetItem::Type);

        QStringList slist;
        for (const std::string &s : profileToInsert.gameMatch.gameCodes)
            slist.append(QString::fromStdString(s));
        profileItem->setText(COL_CODES, slist.join(", "));
        profileItem->setText(COL_NAME, QString::fromStdString(profileToInsert.name));
        profileItem->setText(COL_AUTHOR, QString::fromStdString(profileToInsert.author));
        profileItem->setText(COL_STUDIO, QString::fromStdString(profileToInsert.gameStudio));

        const auto &codes = profileToInsert.gameMatch.gameCodes;
        if (profileToInsert.sessionId == profile->sessionId)
            profileItem->setBackground(COL_CODES, profileColCur);
        else if (gameCode.size() > 0 && std::find(codes.begin(), codes.end(), gameCode) != codes.end())
            profileItem->setBackground(COL_CODES, profileColAvail);
        else
            profileItem->setBackground(COL_CODES, profileColUnavail);

        profileItem->setData(0, Qt::UserRole, profileToInsert.sessionId);
    };

    ui->treeWidget->clear();
    ui->treeWidget->setColumnCount(COL_COUNT);
    ui->treeWidget->setHeaderLabels(QStringList{"Game Code", "Name", "Author", "Game Studio"});
    ui->treeWidget->setColumnWidth(COL_CODES, 90);

    for (std::shared_ptr<Profile> &profileToInsert : pm.GetAllProfiles())
        addProfileToTreeWidget(*profileToInsert);

    ui->treeWidget->setRootIsDecorated(false);
    ui->treeWidget->setSortingEnabled(true);
    ui->treeWidget->sortByColumn(COL_CODES, Qt::AscendingOrder);

    /* load profile button */
    connect(ui->pushButtonProfileLoad, &QAbstractButton::clicked, [this](bool){
        // TODO
    });

    /* copy profile button */
    auto selectProfilePath = [this]() {
        QFileDialog dialog(this);
        dialog.setFileMode(QFileDialog::AnyFile);
        dialog.setAcceptMode(QFileDialog::AcceptSave);
        dialog.setNameFilter("agbplay Profile (*.json)");
        dialog.setDefaultSuffix("json");
        dialog.setDirectory(QString::fromStdWString(ProfileManager::ProfileUserPath().wstring()));

        if (!dialog.exec())
            return std::filesystem::path();

        assert(dialog.selectedFiles().size() == 1);
        return std::filesystem::path(dialog.selectedFiles().at(0).toStdWString());
    };

    auto createProfile = [this, addProfileToTreeWidget, selectProfilePath](bool copy) {
        const auto profilePath = selectProfilePath();
        if (profilePath.empty())
            return;

        std::shared_ptr<Profile> &newProfile = pm.GetAllProfiles().emplace_back(std::make_shared<Profile>());
        if (gameCode.size() > 0) {
            newProfile->gameMatch.gameCodes.emplace_back(gameCode);
        } else {
            for (const auto &code : profile->gameMatch.gameCodes)
                newProfile->gameMatch.gameCodes.emplace_back(code);
        }
        newProfile->path = profilePath;

        if (copy)
            *newProfile = *profile;

        addProfileToTreeWidget(*newProfile);
    };

    connect(ui->pushButtonProfileCopy, &QAbstractButton::clicked, [this, createProfile](bool){
        createProfile(true);
    });

    /* delete profile button */
    connect(ui->pushButtonProfileDelete, &QAbstractButton::clicked, [this](bool){
        QTreeWidgetItem *item = ui->treeWidget->currentItem();
        if (!item)
            return;

        const uint32_t idToDelete = item->data(0, Qt::UserRole).toUInt();

        auto &profiles = pm.GetAllProfiles();
        auto profileToDeleteIt = profiles.end();
        for (size_t i = 0; i < profiles.size(); i++) {
            if (profiles.at(i)->sessionId == idToDelete) {
                profileToDeleteIt = profiles.begin() + i;
            }
        }

        if (profileToDeleteIt == profiles.end())
            return;

        QMessageBox mbox(
            QMessageBox::Icon::Question,
            "Delete Profile",
            "Are you sure you want to delete the selected profile?",
            QMessageBox::Yes | QMessageBox::No,
            this
        );
        if (mbox.exec() == QMessageBox::No)
            return;

        std::shared_ptr<Profile> profileToDelete = *profileToDeleteIt;

        if (profileToDelete->sessionId == profile->sessionId) {
            QMessageBox mbox2(
                QMessageBox::Icon::Warning,
                "Delete Profile",
                "The profile you want to delete is currently loaded, do you want to proceeed anyway?",
                QMessageBox::Yes | QMessageBox::No,
                this
            );
            if (mbox2.exec() == QMessageBox::No)
                return;
            profileToDelete->dirty = true;
        }

        profiles.erase(profileToDeleteIt);
        delete item;
        std::filesystem::remove(profileToDelete->path);
    });

    /* new profile button */
    connect(ui->pushButtonProfileNew, &QAbstractButton::clicked, [this, createProfile](bool){
        createProfile(false);
    });

}

void ProfileSettingsWindow::InitProfileInfo()
{
    ui->lineEditProfileName->setText(QString::fromStdString(profile->name));
    connect(ui->lineEditProfileName, &QLineEdit::textChanged, [this](auto){ MarkPending(); });
    ui->lineEditProfileAuthor->setText(QString::fromStdString(profile->author));
    connect(ui->lineEditProfileAuthor, &QLineEdit::textChanged, [this](auto){ MarkPending(); });
    ui->lineEditGameStudio->setText(QString::fromStdString(profile->gameStudio));
    connect(ui->lineEditGameStudio, &QLineEdit::textChanged, [this](auto){ MarkPending(); });
    ui->lineEditDescription->setText(QString::fromStdString(profile->description));
    connect(ui->lineEditDescription, &QLineEdit::textChanged, [this](auto){ MarkPending(); });
    ui->plainTextEditNotes->clear();
    ui->plainTextEditNotes->appendPlainText(QString::fromStdString(profile->notes));
    connect(ui->plainTextEditNotes, &QPlainTextEdit::textChanged, [this](){ MarkPending(); });
}

void ProfileSettingsWindow::InitSoundMode()
{
    /* volume */
    if (profile->mp2kSoundModeConfig.vol == MP2KSoundMode::VOL_AUTO) {
        ui->spinBoxVol->setValue(profile->mp2kSoundModeScanned.vol);
        ui->spinBoxVol->setEnabled(false);
        ui->checkBoxVol->setCheckState(Qt::Unchecked);
    } else {
        ui->spinBoxVol->setValue(profile->mp2kSoundModeConfig.vol);
        ui->spinBoxVol->setEnabled(true);
        ui->checkBoxVol->setCheckState(Qt::Checked);
    }

    connect(ui->checkBoxVol, &QCheckBox::checkStateChanged, [this](int state){
        if (state == Qt::Checked) {
            ui->spinBoxVol->setEnabled(true);
        } else {
            ui->spinBoxVol->setValue(profile->mp2kSoundModeScanned.vol);
            ui->spinBoxVol->setEnabled(false);
        }
        MarkPending();
    });

    connect(ui->spinBoxVol, &QSpinBox::valueChanged, [this](int) { MarkPending(); });

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

    connect(ui->checkBoxRev, &QCheckBox::checkStateChanged, [this](int state){
        if (state == Qt::Checked) {
            ui->spinBoxRev->setEnabled(true);
        } else {
            ui->spinBoxRev->setValue(profile->mp2kSoundModeScanned.rev);
            ui->spinBoxRev->setEnabled(false);
        }
        MarkPending();
    });

    connect(ui->spinBoxRev, &QSpinBox::valueChanged, [this](int) { MarkPending(); });

    /* sample rate */
    static const std::array sampleRates{
        5734, 7884, 10512, 13379, 15768, 18157, 21024, 26758, 31536, 36314, 40137, 42048
    };

    for (unsigned int i = 0; i < sampleRates.size(); i++)
        ui->comboBoxFreq->addItem(QString::fromStdString(std::format("{} Hz", sampleRates.at(i))), i + 1);

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

    connect(ui->checkBoxFreq, &QCheckBox::checkStateChanged, [this](int state){
        if (state == Qt::Checked) {
            ui->comboBoxFreq->setEnabled(true);
        } else {
            ui->comboBoxFreq->setCurrentIndex(profile->mp2kSoundModeScanned.freq - 1);
            ui->comboBoxFreq->setEnabled(false);
        }
        MarkPending();
    });

    connect(ui->comboBoxFreq, &QComboBox::currentIndexChanged, [this](int) { MarkPending(); });

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

    connect(ui->checkBoxChn, &QCheckBox::checkStateChanged, [this](int state){
        if (state == Qt::Checked) {
            ui->spinBoxChn->setEnabled(true);
        } else {
            ui->spinBoxChn->setValue(profile->mp2kSoundModeScanned.maxChannels);
            ui->spinBoxChn->setEnabled(false);
        }
        MarkPending();
    });

    connect(ui->spinBoxChn, &QSpinBox::valueChanged, [this](int) { MarkPending(); });

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

    connect(ui->checkBoxDac, &QCheckBox::checkStateChanged, [this](int state){
        if (state == Qt::Checked) {
            ui->comboBoxDac->setEnabled(true);
        } else {
            ui->comboBoxDac->setCurrentIndex(profile->mp2kSoundModeScanned.dacConfig % 4);
            ui->comboBoxDac->setEnabled(false);
        }
        MarkPending();
    });

    connect(ui->comboBoxDac, &QComboBox::currentIndexChanged, [this](int) { MarkPending(); });
}

void ProfileSettingsWindow::InitEnhancements()
{
    /* Resampler combo boxes */
    for (QComboBox *comboBox : { ui->comboBoxResTypeNormal, ui->comboBoxResTypeFixed }) {
        comboBox->clear();
        comboBox->addItem("Nearest (fast)", static_cast<int>(ResamplerType::NEAREST));
        comboBox->addItem("Linear (fast)", static_cast<int>(ResamplerType::LINEAR));
        comboBox->addItem("Sinc (slow)", static_cast<int>(ResamplerType::SINC));
        comboBox->addItem("Blep (slow)", static_cast<int>(ResamplerType::BLEP));
        comboBox->addItem("Blamp (slow)", static_cast<int>(ResamplerType::BLAMP));
    }

    ui->comboBoxResTypeNormal->setCurrentIndex(static_cast<int>(profile->agbplaySoundMode.resamplerTypeNormal));
    ui->comboBoxResTypeFixed->setCurrentIndex(static_cast<int>(profile->agbplaySoundMode.resamplerTypeFixed));

    static const QString resToolTip = "Specify resampler type:\n"
        "- Nearest sounds harsh, while linear sounds a bit smoother.\n"
        "- However, both lower quality due to aliasing artifacts.\n"
        "- Sinc has least aliasing, but may sound muffled.\n"
        "- Blep mimics the sound of nearest, but uses bandlimited rectangular pulses, which avoids aliasing.\n"
        "- Blamp mimics the sound of linear, but uses bandlimited triangular pulses, which avoids aliasing.";

    ui->comboBoxResTypeNormal->setToolTip(resToolTip);
    ui->comboBoxResTypeFixed->setToolTip(resToolTip);

    connect(ui->pushButtonResTypeNormal, &QPushButton::clicked, [this](bool){
        ui->comboBoxResTypeNormal->setCurrentIndex(static_cast<int>(ResamplerType::BLAMP));
        MarkPending();
    });

    connect(ui->pushButtonResTypeFixed, &QPushButton::clicked, [this](bool){
        ui->comboBoxResTypeFixed->setCurrentIndex(static_cast<int>(ResamplerType::BLEP));
        MarkPending();
    });

    connect(ui->comboBoxResTypeNormal, &QComboBox::currentIndexChanged, [this](int) { MarkPending(); });
    connect(ui->comboBoxResTypeFixed, &QComboBox::currentIndexChanged, [this](int) { MarkPending(); });

    /* reverb type */
    ui->comboBoxRevAlgo->clear();
    ui->comboBoxRevAlgo->addItem("Normal", static_cast<int>(ReverbType::NORMAL));
    ui->comboBoxRevAlgo->addItem("Golden Sun", static_cast<int>(ReverbType::GS1));
    ui->comboBoxRevAlgo->addItem("Golden Sun - The Lost Age", static_cast<int>(ReverbType::GS2));
    ui->comboBoxRevAlgo->addItem("Mario Golf - Advance Tour", static_cast<int>(ReverbType::MGAT));
    ui->comboBoxRevAlgo->addItem("Test (development only)", static_cast<int>(ReverbType::TEST));
    ui->comboBoxRevAlgo->addItem("None", static_cast<int>(ReverbType::NONE));

    ui->comboBoxRevAlgo->setCurrentIndex(static_cast<int>(profile->agbplaySoundMode.reverbType));

    static const QString revToolTip = "Specify reverb algorithm:\n"
        "- 'Normal' is the standard reverb used in most games. This is the only algorithm affected by depth setting.\n"
        "- 'Golden Sun', 'Goldeun Sun - The Lost Age', and 'Mario Golf - Advance Tour' are the algorithms of their respective games.\n"
        "- 'Mario Tennis - Power Tour' uses the same reverb algorithm as used in 'Mario Golf - Advance Tour'\n"
        "- 'Test' should be used for development purpose only.";

    ui->comboBoxRevAlgo->setToolTip(revToolTip);

    connect(ui->pushButtonRevAlgo, &QPushButton::clicked, [this](bool){
        ui->comboBoxRevAlgo->setCurrentIndex(static_cast<int>(ReverbType::NORMAL));
        MarkPending();
    });

    connect(ui->comboBoxRevAlgo, &QComboBox::currentIndexChanged, [this](int) { MarkPending(); });

    /* PSG polyphony */
    ui->comboBoxPsgPoly->clear();
    ui->comboBoxPsgPoly->addItem("Monophonic (strict)", static_cast<int>(CGBPolyphony::MONO_STRICT));
    ui->comboBoxPsgPoly->addItem("Monophonic (smooth)", static_cast<int>(CGBPolyphony::MONO_SMOOTH));
    ui->comboBoxPsgPoly->addItem("Polyphonic", static_cast<int>(CGBPolyphony::POLY));

    static const QString psgPolyToopTip = "Specify polyphony for PSGs:\n"
        "- Monophonic (strict) hard limits polyphony to one note.\n"
        "- Monophonic (smooth) limits polyphony to one note, but allows for a short smooth transition.\n"
        "  This allows for a less abrupt transition between two PSG notes, but can sometimes cause phasing artifacts.\n"
        "- Polyphonic allows unlimited simultaneous PSG notes.";

    ui->comboBoxPsgPoly->setToolTip(psgPolyToopTip);

    ui->comboBoxPsgPoly->setCurrentIndex(static_cast<int>(profile->agbplaySoundMode.cgbPolyphony));

    connect(ui->pushButtonPsgPoly, &QAbstractButton::clicked, [this](bool){
        ui->comboBoxPsgPoly->setCurrentIndex(static_cast<int>(CGBPolyphony::MONO_STRICT));
        MarkPending();
    });

    connect(ui->comboBoxPsgPoly, &QComboBox::currentIndexChanged, [this](int) { MarkPending(); });


    /* PCM DMA buffer size */
    ui->spinBoxDmaBufLen->setValue(static_cast<int>(profile->agbplaySoundMode.dmaBufferLen));

    static const QString dmaBufLenToolTip = "Specify PCM DMA reverb buffer len:\n"
        "This is 1584 (0x630) for all known commercial GBA games.\n"
        "Increasing this increases the duration of the reverb echoes fromt the 'Normal' reverb type";

    ui->spinBoxDmaBufLen->setToolTip(dmaBufLenToolTip);

    connect(ui->pushButtonDmaBufLen, &QPushButton::clicked, [this](bool){
        ui->spinBoxDmaBufLen->setValue(0x630);  // TODO change this to a more global define
        MarkPending();
    });

    connect(ui->spinBoxDmaBufLen, &QSpinBox::valueChanged, [this](int) { MarkPending(); });
    /* accurate ch3 quantization */
    ui->checkBoxCh3Quant->setCheckState(profile->agbplaySoundMode.accurateCh3Quantization ? Qt::Checked : Qt::Unchecked);

    static const QString quantToolTip = "Accurate PSG CH3 quantization:\n"
        "On real GBA hardware, the volume of PSG CH3 will affect the quantization, thus change the timbre.\n"
        "Disable this to get 'better than hardware' sound, but for low volumes it may noticeably change the timbre.";

    ui->checkBoxCh3Quant->setToolTip(quantToolTip);

    connect(ui->pushButtonCh3Quant, &QPushButton::clicked, [this](bool){
        ui->checkBoxCh3Quant->setCheckState(Qt::Checked);
        MarkPending();
    });

    connect(ui->checkBoxCh3Quant, &QCheckBox::checkStateChanged, [this](int) { MarkPending(); });

    /* accurate ch3 volume */
    ui->checkBoxCh3Vol->setCheckState(profile->agbplaySoundMode.accurateCh3Volume ? Qt::Checked : Qt::Unchecked);

    static const QString volToolTip = "On real GBA hardware, the PSG CH3 volume is limited to 0%, 25%, 50%, 75%, an 100%.\n"
        "Disable this to get full 16 steps of volume like other PSG channels.";

    ui->checkBoxCh3Vol->setToolTip(volToolTip);

    connect(ui->pushButtonCh3Vol, &QPushButton::clicked, [this](bool){
        ui->checkBoxCh3Vol->setCheckState(Qt::Checked);
        MarkPending();
    });

    connect(ui->checkBoxCh3Vol, &QCheckBox::checkStateChanged, [this](int) { MarkPending(); });

    /* emulate PSG sustain bug */
    ui->checkBoxPsgSus->setCheckState(profile->agbplaySoundMode.emulateCgbSustainBug ? Qt::Checked : Qt::Unchecked);

    static const QString susToolTip = "The original engine code has a bug, which causes the volume of all PSGs to not be updated correctly during sustain.\n"
        "Disable this to correct volume/sustain according to song data.\n"
        "However, some songs may run into this bug quite often and will have excessive or silent volume at specific times.";

    ui->checkBoxPsgSus->setToolTip(susToolTip);

    connect(ui->pushButtonPsgSus, &QPushButton::clicked, [this](bool){
        ui->checkBoxPsgSus->setCheckState(Qt::Checked);
        MarkPending();
    });

    connect(ui->checkBoxPsgSus, &QCheckBox::checkStateChanged, [this](int) { MarkPending(); });
}

void ProfileSettingsWindow::InitGameTables()
{
    /* songtable pos & index */
    if (profile->songTableInfoConfig.pos == SongTableInfo::POS_AUTO) {
        ui->spinBoxSongTable->setValue(static_cast<int>(profile->songTableInfoScanned.pos));
        ui->checkBoxSongTable->setCheckState(Qt::Unchecked);
        ui->spinBoxSongTable->setEnabled(false);
        ui->spinBoxTableIndex->setValue(profile->songTableInfoScanned.tableIdx);
        ui->spinBoxTableIndex->setEnabled(true);
    } else {
        ui->spinBoxSongTable->setValue(static_cast<int>(profile->songTableInfoConfig.pos));
        ui->checkBoxSongTable->setCheckState(Qt::Checked);
        ui->spinBoxSongTable->setEnabled(true);
        ui->spinBoxTableIndex->setValue(profile->songTableInfoConfig.tableIdx);
        ui->spinBoxTableIndex->setEnabled(false);
    }

    connect(ui->checkBoxSongTable, &QCheckBox::checkStateChanged, [this](int state){
        if (state == Qt::Checked) {
            ui->spinBoxSongTable->setEnabled(true);
            ui->checkBoxSongCount->setCheckState(Qt::Checked);
            ui->checkBoxSongCount->setEnabled(false);
            ui->spinBoxTableIndex->setEnabled(false);
        } else {
            ui->spinBoxSongTable->setValue(static_cast<int>(profile->songTableInfoScanned.pos));
            ui->spinBoxSongTable->setEnabled(false);
            ui->checkBoxSongCount->setCheckState(Qt::Unchecked);
            ui->checkBoxSongCount->setEnabled(true);
            ui->spinBoxTableIndex->setValue(profile->songTableInfoScanned.tableIdx);
            ui->spinBoxTableIndex->setEnabled(true);
        }
        MarkPending();
    });

    connect(ui->spinBoxSongTable, &QSpinBox::valueChanged, [this](auto){ MarkPending(); });
    connect(ui->spinBoxTableIndex, &QSpinBox::valueChanged, [this](auto){ MarkPending(); });

    /* song  entry count */
    if (profile->songTableInfoConfig.count == SongTableInfo::COUNT_AUTO) {
        ui->spinBoxSongCount->setValue(profile->songTableInfoScanned.count);
        ui->checkBoxSongCount->setCheckState(Qt::Unchecked);
        ui->spinBoxSongCount->setEnabled(false);
    } else {
        ui->spinBoxSongCount->setValue(profile->songTableInfoConfig.count);
        ui->checkBoxSongCount->setCheckState(Qt::Checked);
        ui->spinBoxSongCount->setEnabled(true);
    }

    connect(ui->checkBoxSongCount, &QCheckBox::checkStateChanged, [this](int state){
        if (state == Qt::Checked) {
            ui->spinBoxSongCount->setEnabled(true);
        } else {
            ui->spinBoxSongCount->setValue(profile->songTableInfoScanned.count);
            ui->spinBoxSongCount->setEnabled(false);
        }
        MarkPending();
    });

    connect(ui->spinBoxSongCount, &QSpinBox::valueChanged, [this](auto){ MarkPending(); });

    /* player table */
    ui->tableWidgetPlayers->setSelectionMode(QAbstractItemView::NoSelection);
    ui->tableWidgetPlayers->setColumnCount(COL_PLT_COUNT);
    ui->tableWidgetPlayers->setHorizontalHeaderItem(COL_PLT_TRACKS, new QTableWidgetItem("Max Tracks"));
    ui->tableWidgetPlayers->setHorizontalHeaderItem(COL_PLT_PRIO, new QTableWidgetItem("Use Priority"));

    connect(ui->tableWidgetPlayers, &QTableWidget::cellChanged, [this](int row, int column) {
        /* We validate the content:
         * Remove non-number characters, except a blank string, which is equal to 0. */
        assert(column < COL_PLT_COUNT);

        QTableWidgetItem *item = ui->tableWidgetPlayers->item(row, column);
        assert(item);

        // TODO use global define mor maximum tracks of 16
        int limit = 0;
        if (column == COL_PLT_TRACKS) {
            limit = 16;
        } else {
            limit = 1;
        }
        QString newString = item->text();
        (void)newString.remove(QRegularExpression("[^0-9]"));
        if (newString.size() == 0)
            newString = "0";
        if (newString.toInt() > limit)
            newString = QString::number(limit);
        item->setText(newString);
        MarkPending();
    });

    auto populateTable = [this](const PlayerTableInfo &plt) {
        ui->tableWidgetPlayers->setRowCount(0);
        for (size_t i = 0; i < plt.size(); i++) {
            const int row = ui->tableWidgetPlayers->rowCount();
            ui->tableWidgetPlayers->insertRow(row);
            ui->tableWidgetPlayers->setItem(row, COL_PLT_TRACKS, new QTableWidgetItem(QString::number(plt.at(i).maxTracks)));
            ui->tableWidgetPlayers->setItem(row, COL_PLT_PRIO, new QTableWidgetItem(QString::number(plt.at(i).usePriority)));
            ui->tableWidgetPlayers->setVerticalHeaderItem(row, new QTableWidgetItem(QString::fromStdString(std::format("Player {}", row))));
        }
    };

    if (profile->playerTableConfig.size() == 0) {
        populateTable(profile->playerTableScanned);
        ui->groupBoxPlayerTable->setChecked(false);
    } else {
        populateTable(profile->playerTableConfig);
        ui->groupBoxPlayerTable->setChecked(true);
    }

    connect(ui->groupBoxPlayerTable, &QGroupBox::clicked, [this, populateTable](bool checked) {
        if (!checked)
            populateTable(profile->playerTableScanned);
        MarkPending();
    });

    connect(ui->pushButtonPlayerAdd, &QPushButton::clicked, [this](bool) {
        const int row = ui->tableWidgetPlayers->rowCount();
        // TODO replace 16 with constant of max players
        if (row >= 32)
            return;
        ui->tableWidgetPlayers->insertRow(row);
        ui->tableWidgetPlayers->setItem(row, COL_PLT_TRACKS, new QTableWidgetItem("16")); // TODO replace 16 with constant
        ui->tableWidgetPlayers->setItem(row, COL_PLT_PRIO, new QTableWidgetItem("0"));
        ui->tableWidgetPlayers->setVerticalHeaderItem(row, new QTableWidgetItem(QString::fromStdString(std::format("Player {}", row))));
        MarkPending();
    });

    connect(ui->pushButtonPlayerRemove, &QPushButton::clicked, [this](bool) {
        const int rows = ui->tableWidgetPlayers->rowCount();
        if (rows <= 1)
            return;
        ui->tableWidgetPlayers->removeRow(rows - 1);
        MarkPending();
    });
}

void ProfileSettingsWindow::InitProfileAssignment()
{
    /* game codes */
    static const auto defaultFlags = Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren | Qt::ItemIsEnabled;

    ui->listWidgetGameCodes->clear();
    for (const std::string &c : profile->gameMatch.gameCodes) {
        QListWidgetItem *item = nullptr;
        try {
            item = new QListWidgetItem(QString::fromStdString(c));
            item->setFlags(defaultFlags);
            ui->listWidgetGameCodes->addItem(item);
        } catch (...) {
            delete item;
            throw;
        }
    }

    connect(ui->listWidgetGameCodes, &QListWidget::itemChanged, [this](QListWidgetItem *item){
        /* only allow game codes with capital letters and numbers up to 4 length. */
        QString oldText = item->text();
        QString newText = oldText; // We have to copy string since remove() appears to be destructive
        (void)newText.remove(QRegularExpression("[^A-Z0-9]"));
        newText = newText.left(4);
        if (newText != oldText)
            item->setText(newText);
        MarkPending();
    });

    connect(ui->pushButtonCodeAdd, &QAbstractButton::clicked, [this](bool) {
        QListWidgetItem *item = nullptr;
        try {
            item = new QListWidgetItem("0000");
            item->setFlags(defaultFlags);
            ui->listWidgetGameCodes->addItem(item);
        } catch (...) {
            delete item;
            throw;
        }
        MarkPending();
    });

    connect(ui->pushButtonCodeRemove, &QAbstractButton::clicked, [this](bool) {
        auto items = ui->listWidgetGameCodes->selectedItems();
        if (items.size() == 0) {
            QMessageBox mbox(QMessageBox::Icon::Critical, "Game Code Matches", "Please select a game code to remove", QMessageBox::Ok, this);
            mbox.exec();
            return;
        }

        if (ui->listWidgetGameCodes->count() <= 1) {
            QMessageBox mbox(QMessageBox::Icon::Critical, "Game Code Matches", "Cannot unassign last remaining game code", QMessageBox::Ok, this);
            mbox.exec();
            return;
        }

        for (QListWidgetItem *item : items)
            ui->listWidgetGameCodes->takeItem(ui->listWidgetGameCodes->row(item));
        MarkPending();
    });

    /* magic bytes */
    QStringList byteStringList;
    for (uint8_t byte : profile->gameMatch.magicBytes)
        byteStringList << QString::fromStdString(std::format("{:02X}", byte));
    ui->lineEditMagicBytes->setText(byteStringList.join(" "));
    prevMagicBytes = ui->lineEditMagicBytes->text();

    connect(ui->lineEditMagicBytes, &QLineEdit::textEdited, [this](const QString &text){
        /* make sure the string stays in format '04 AC 8D FF 11 ...' */
        const QString oldText = text;
        
        QString nospaceText;
        int cursorPos = -1;

        /* 1. remove all illegal characters and save adjuted cursor position. */
        for (int i = 0; i < oldText.size(); i++) {
            bool deleteSpace = false;
            if (i == ui->lineEditMagicBytes->cursorPosition()) {
                if (i < prevMagicBytes.size() && prevMagicBytes.at(i) == QChar(' '))
                    deleteSpace = true;
                cursorPos = static_cast<int>(nospaceText.size());
            }

            const QChar &c = oldText.at(i);
            static const QString allowedChars = "0123456789ABCDEF";

            if (!allowedChars.contains(c, Qt::CaseInsensitive))
                continue;

            /* BUG: We cannot determine here if delete or backspace was pressed.
             * The current behavior is correct for backspace, but if delete key is pressed,
             * we would have to skip inserting 'c' instead of erasing the previously added
             * character. */
            if (deleteSpace && nospaceText.size() > 0)
                nospaceText.erase(nospaceText.end() - 1);

            nospaceText += c.toUpper();
        }

        QString newText;
        bool odd = false;

        /* 2. insert spaces according to new formatting and set cursor appropriately. */
        for (const QChar &c : nospaceText) {
            if (!odd && newText.size() > 0)
                newText += " ";
            newText += c;
            odd = !odd;
        }

        /* 3. set text and cursor if it has actually changed. */
        if (newText != oldText) {
            ui->lineEditMagicBytes->setText(newText);
            if (cursorPos >= 0)
                ui->lineEditMagicBytes->setCursorPosition(cursorPos);
        }

        prevMagicBytes = ui->lineEditMagicBytes->text();
    });
}

void ProfileSettingsWindow::MarkPending()
{
    pendingChanges = true;
}

void ProfileSettingsWindow::ClearPending()
{
    pendingChanges = false;
}

void ProfileSettingsWindow::UpdateProfileList()
{
}

void ProfileSettingsWindow::Apply()
{
    if (!pendingChanges)
        return;

    /* profile info */
    profile->name = ui->lineEditProfileName->text().toStdString();
    profile->author = ui->lineEditProfileAuthor->text().toStdString();
    profile->gameStudio = ui->lineEditGameStudio->text().toStdString();
    profile->description = ui->lineEditDescription->text().toStdString();
    profile->notes = ui->plainTextEditNotes->toPlainText().toStdString();

    /* sound mode */
    if (ui->checkBoxVol->checkState() == Qt::Checked)
        profile->mp2kSoundModeConfig.vol = static_cast<uint8_t>(ui->spinBoxVol->value());
    else
        profile->mp2kSoundModeConfig.vol = MP2KSoundMode::VOL_AUTO;

    if (ui->checkBoxRev->checkState() == Qt::Checked)
        profile->mp2kSoundModeConfig.rev = static_cast<uint8_t>(ui->spinBoxRev->value());
    else
        profile->mp2kSoundModeConfig.rev = MP2KSoundMode::REV_AUTO;

    if (ui->checkBoxFreq->checkState() == Qt::Checked)
        profile->mp2kSoundModeConfig.freq = static_cast<uint8_t>(ui->comboBoxFreq->currentData().toInt());
    else
        profile->mp2kSoundModeConfig.freq = MP2KSoundMode::FREQ_AUTO;

    if (ui->checkBoxChn->checkState() == Qt::Checked)
        profile->mp2kSoundModeConfig.maxChannels = static_cast<uint8_t>(ui->spinBoxChn->value());
    else
        profile->mp2kSoundModeConfig.maxChannels = MP2KSoundMode::CHN_AUTO;

    if (ui->checkBoxDac->checkState() == Qt::Checked)
        profile->mp2kSoundModeConfig.dacConfig = static_cast<uint8_t>(ui->comboBoxDac->currentData().toInt());
    else
        profile->mp2kSoundModeConfig.dacConfig = MP2KSoundMode::DAC_AUTO;

    /* enhancements */
    profile->agbplaySoundMode.resamplerTypeNormal = static_cast<ResamplerType>(ui->comboBoxResTypeNormal->currentData().toInt());
    profile->agbplaySoundMode.resamplerTypeFixed = static_cast<ResamplerType>(ui->comboBoxResTypeFixed->currentData().toInt());
    profile->agbplaySoundMode.reverbType = static_cast<ReverbType>(ui->comboBoxRevAlgo->currentData().toInt());
    profile->agbplaySoundMode.cgbPolyphony = static_cast<CGBPolyphony>(ui->comboBoxPsgPoly->currentData().toInt());
    profile->agbplaySoundMode.dmaBufferLen = static_cast<uint32_t>(ui->spinBoxDmaBufLen->value());
    profile->agbplaySoundMode.accurateCh3Quantization = ui->checkBoxCh3Quant->checkState() == Qt::Checked;
    profile->agbplaySoundMode.accurateCh3Volume = ui->checkBoxCh3Vol->checkState() == Qt::Checked;
    profile->agbplaySoundMode.emulateCgbSustainBug = ui->checkBoxPsgSus->checkState() == Qt::Checked;

    /* game tables (song table and player table) */
    if (ui->checkBoxSongTable->checkState() == Qt::Checked) {
        profile->songTableInfoConfig.pos = static_cast<size_t>(ui->spinBoxSongTable->value());
        profile->songTableInfoConfig.tableIdx = 0;
    } else {
        profile->songTableInfoConfig.pos = SongTableInfo::POS_AUTO;
        profile->songTableInfoConfig.tableIdx = static_cast<uint8_t>(ui->spinBoxTableIndex->value());
    }

    if (ui->checkBoxSongCount->checkState() == Qt::Checked)
        profile->songTableInfoConfig.count = static_cast<uint16_t>(ui->spinBoxSongCount->value());
    else
        profile->songTableInfoConfig.count = SongTableInfo::COUNT_AUTO;

    if (ui->groupBoxPlayerTable->isChecked()) {
        profile->playerTableConfig.clear();
        for (int row = 0; row < ui->tableWidgetPlayers->rowCount(); row++) {
            QTableWidgetItem *maxTracksItem = ui->tableWidgetPlayers->item(row, COL_PLT_TRACKS);
            QTableWidgetItem *usePrioItem = ui->tableWidgetPlayers->item(row, COL_PLT_PRIO);
            const uint8_t maxTracks = static_cast<uint8_t>(maxTracksItem->text().toInt());
            const uint8_t usePrio = static_cast<uint8_t>(usePrioItem->text().toInt());
            profile->playerTableConfig.emplace_back(maxTracks, usePrio);
        }
    } else {
        profile->playerTableConfig.clear();
    }

    /* profile assignments */
    profile->gameMatch.gameCodes.clear();
    for (int i = 0; i < ui->listWidgetGameCodes->count(); i++) {
        QListWidgetItem *item = ui->listWidgetGameCodes->item(i);
        profile->gameMatch.gameCodes.emplace_back(item->text().toStdString());
    }

    profile->gameMatch.magicBytes.clear();
    QStringList byteStrList = ui->lineEditMagicBytes->text().split(" ");
    for (const QString &byteStr : byteStrList) {
        const uint8_t byte = static_cast<uint8_t>(byteStr.toInt(nullptr, 16));
        profile->gameMatch.magicBytes.emplace_back(byte);
    }

    // TODO apply config+scan to playback, or should we do that somewhere else?
    profile->dirty = true;
    ClearPending();
}
