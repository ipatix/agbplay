#include "SettingsWindow.hpp"
#include "Settings.hpp"
#include "ui_SettingsWindow.h"

#include <algorithm>
#include <cstdint>
#include <fmt/core.h>
#include <QFileDialog>
#include <QInputDialog>
#include <QStandardPaths>
#include <vector>

static const std::vector<uint32_t> standardRates = {22050, 32000, 44100, 48000, 96000, 192000};
static const uint32_t RATE_CUSTOM = 0u;
static const std::vector<uint32_t> standardBits = {16, 24, 32};

SettingsWindow::SettingsWindow(QWidget *parent, Settings &settings) :
QDialog(parent), ui(new Ui::SettingsWindow), settings(settings)
{
    ui->setupUi(this);

    /* init samplerate combo boxes */
    for (const auto v : standardRates) {
        const auto s = QString::fromStdString(fmt::format("{} Hz", v));
        ui->playbackSampleRateComboBox->addItem(s, QVariant(v));
        ui->exportSampleRateComboBox->addItem(s, QVariant(v));
    }

    ui->playbackSampleRateComboBox->addItem("(add custom)", QVariant(RATE_CUSTOM));
    ui->exportSampleRateComboBox->addItem("(add custom)", QVariant(RATE_CUSTOM));

    auto it = std::find(standardRates.begin(), standardRates.end(), settings.playbackSampleRate);
    if (it == standardRates.end()) {
        /* manually add samplerate if not in the list */
        const auto r = settings.playbackSampleRate;
        const auto s = QString::fromStdString(fmt::format("{} Hz (custom)", r));
        ui->playbackSampleRateComboBox->addItem(s, QVariant(r));
    }

    playbackComboBoxIndex = ui->playbackSampleRateComboBox->findData(QVariant(settings.playbackSampleRate));
    assert(playbackComboBoxIndex >= 0);
    ui->playbackSampleRateComboBox->setCurrentIndex(playbackComboBoxIndex);

    it = std::find(standardRates.begin(), standardRates.end(), settings.exportSampleRate);
    if (it == standardRates.end()) {
        /* manually add samplerate if not in the list */
        const auto r = settings.exportSampleRate;
        const auto s = QString::fromStdString(fmt::format("{} Hz (custom)", r));
        ui->exportSampleRateComboBox->addItem(s, QVariant(r));
    }

    exportComboBoxIndex = ui->exportSampleRateComboBox->findData(QVariant(settings.exportSampleRate));
    assert(exportComboBoxIndex >= 0);
    ui->exportSampleRateComboBox->setCurrentIndex(exportComboBoxIndex);

    /* init bit depth combo box */
    for (const auto v : standardBits) {
        const auto s = QString::fromStdString(fmt::format("{}-bit", v));
        ui->exportBitDepthComboBox->addItem(s, QVariant(v));
    }

    it = std::find(standardBits.begin(), standardBits.end(), settings.exportBitDepth);
    exportComboBoxIndex = ui->exportBitDepthComboBox->findData(QVariant(settings.exportBitDepth));
    ui->exportBitDepthComboBox->setCurrentIndex(exportComboBoxIndex);

    /* init other fields */
    ui->exportPadStartSpinBox->setValue(settings.exportPadStart);
    ui->exportPadEndSpinBox->setValue(settings.exportPadEnd);

    ui->exportFolderGroupBox->setChecked(!settings.exportQuickExportAsk);
    ui->exportFolderLineEdit->setText(QString::fromStdWString(settings.exportQuickExportDirectory.wstring()));

    /* setup handlers */
    connect(ui->playbackSampleRateComboBox, &QComboBox::activated, this, &SettingsWindow::playbackComboBoxActivated);
    connect(ui->exportSampleRateComboBox, &QComboBox::activated, this, &SettingsWindow::exportComboBoxActivated);
    connect(ui->exportFolderGroupBox, &QGroupBox::toggled, [this](bool on) {
        ui->exportFolderLineEdit->setEnabled(on);
        ui->exportFolderPushButton->setEnabled(on);
    });
    connect(ui->exportFolderPushButton, &QPushButton::clicked, this, &SettingsWindow::exportPushButtonPressed);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &SettingsWindow::buttonBoxButtonPressed);
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
}

void SettingsWindow::playbackComboBoxActivated(int index)
{
    updateComboBoxRate(ui->playbackSampleRateComboBox, playbackComboBoxIndex, index);
}

void SettingsWindow::exportComboBoxActivated(int index)
{
    updateComboBoxRate(ui->exportSampleRateComboBox, exportComboBoxIndex, index);
}

void SettingsWindow::updateComboBoxRate(QComboBox *comboBox, int &index, const int indexActivated)
{
    unsigned int newRate = comboBox->itemData(indexActivated).toUInt();
    unsigned int oldRate = comboBox->itemData(index).toUInt();

    if (newRate == RATE_CUSTOM) {
        bool ok = false;
        newRate = static_cast<uint32_t>(QInputDialog::getInt(
            this, "Please select playback samplerate", "Samplerate:", static_cast<int32_t>(oldRate), 1, 768000, 1, &ok
        ));

        if (!ok) {
            const int oldIndex = comboBox->findData(QVariant(oldRate));
            if (oldIndex >= 0) {
                comboBox->setCurrentIndex(oldIndex);
            } else {
                /* not super critical failure, but should not occur nevertheless */
                assert(false);
            }
            return;
        }
    }

    int newIndex = comboBox->findData(QVariant(newRate));
    const int oldIndex = index;

    const auto newIt = std::find(standardRates.begin(), standardRates.end(), newRate);
    const auto oldIt = std::find(standardRates.begin(), standardRates.end(), oldRate);

    if (oldIt != standardRates.end() && newIt != standardRates.end()) {
        /* Old and new rate are standard rates, no need to change combo box. */
        assert(newIndex >= 0);
    } else if (oldIt != standardRates.end() && newIt == standardRates.end()) {
        /* Only old rate is standard rate, add custom rate to combo box. */
        assert(newIndex < 0);
        const auto s = QString::fromStdString(fmt::format("{} Hz (custom)", newRate));
        comboBox->addItem(s, newRate);
        newIndex = comboBox->findData(newRate);
        assert(newIndex >= 0);
    } else if (oldIt == standardRates.end() && newIt != standardRates.end()) {
        /* Only new rate is standard rate, delete current custom rate. */
        assert(newIndex >= 0);
        comboBox->removeItem(oldIndex);
    } else {
        /* Both old and new rates are custom. Update existing custom rate. */
        const auto s = QString::fromStdString(fmt::format("{} Hz (custom)", newRate));
        comboBox->setItemText(oldIndex, s);
        comboBox->setItemData(oldIndex, newRate);
        newIndex = oldIndex;
    }

    comboBox->setCurrentIndex(newIndex);
    index = newIndex;
}

void SettingsWindow::exportPushButtonPressed(bool)
{
    QFileDialog fileDialog{this};
    fileDialog.setFileMode(QFileDialog::Directory);
    const QString initDir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    if (!initDir.isEmpty())
        fileDialog.setDirectory(initDir);

    if (!fileDialog.exec())
        return;

    assert(fileDialog.selectedFiles().size() == 1);

    ui->exportFolderLineEdit->setText(fileDialog.selectedFiles().at(0));
}

void SettingsWindow::buttonBoxButtonPressed(QAbstractButton *button)
{
    const auto *okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    const auto *applyButton = ui->buttonBox->button(QDialogButtonBox::Apply);

    if (button != okButton && button != applyButton)
        return;

    /* if OK or apply was pressed, apply changes to settings */
    settings.playbackSampleRate = std::max(1u, ui->playbackSampleRateComboBox->currentData().toUInt());
    settings.exportSampleRate = std::max(1u, ui->exportSampleRateComboBox->currentData().toUInt());
    settings.exportBitDepth = std::max(1u, ui->exportBitDepthComboBox->currentData().toUInt());
    settings.exportPadStart = std::clamp(ui->exportPadStartSpinBox->value(), 0.0, 100.0);
    settings.exportPadEnd = std::clamp(ui->exportPadEndSpinBox->value(), 0.0, 100.0);
    settings.exportQuickExportDirectory = ui->exportFolderLineEdit->text().toStdWString();
    settings.exportQuickExportAsk = !ui->exportFolderGroupBox->isChecked();
    settings.Save();
}
