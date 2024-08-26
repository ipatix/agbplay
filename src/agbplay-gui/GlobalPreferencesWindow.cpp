#include "GlobalPreferencesWindow.h"

GlobalPreferencesWindow::GlobalPreferencesWindow(QWidget *parent)
    : QDialog(parent)
{
    layout.addWidget(&playbackGroupBox);
    layout.addWidget(&exportGroupBox);
    layout.addLayout(&botButtonLayout);

    playbackLayout.addWidget(&playbackSampleRateLabel, 0, 0);
    playbackLayout.addWidget(&playbackSampleRateComboBox, 0, 1);

    playbackLayout.setColumnStretch(0, 1);
    playbackLayout.setColumnStretch(1, 1);

    exportLayout.addWidget(&exportSampleRateLabel, 0, 0, 1, 2);
    exportLayout.addWidget(&exportSampleRateComboBox, 0, 2, 1, 2);
    exportLayout.addWidget(&exportQuickExportDirLabel, 1, 0, 1, 3);
    exportLayout.addWidget(&exportQuickExportDirCheckBox, 1, 3, 1, 1);
    exportLayout.addWidget(&exportQuickExportDirLineEdit, 2, 0, 1, 3);
    exportLayout.addWidget(&exportQuickExportDirButton, 2, 3, 1, 1);

    //exportLayout.setColumnStretch(0, 1);
    //exportLayout.setColumnStretch(1, 3);

    botButtonLayout.addWidget(&saveButton);
    botButtonLayout.addWidget(&cancelButton);

    exportQuickExportDirLabel.setMinimumHeight(25);
    exportQuickExportDirButton.setFixedWidth(30);
    exportQuickExportDirLineEdit.setReadOnly(true);
}

GlobalPreferencesWindow::~GlobalPreferencesWindow()
{
}
