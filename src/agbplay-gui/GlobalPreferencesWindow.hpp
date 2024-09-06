#pragma once

#include <QDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>

class GlobalPreferencesWindow : public QDialog 
{
    Q_OBJECT

public:
    GlobalPreferencesWindow(QWidget *parent = nullptr);
    ~GlobalPreferencesWindow() override;

private:
    QVBoxLayout layout{this};

    QGroupBox playbackGroupBox{"Playback Settings", this};
    QGridLayout playbackLayout{&playbackGroupBox};
    QLabel playbackSampleRateLabel{"Sample Rate", &playbackGroupBox};
    QComboBox playbackSampleRateComboBox{&playbackGroupBox};

    QGroupBox exportGroupBox{"Export Settings", this};
    QGridLayout exportLayout{&exportGroupBox};
    QLabel exportSampleRateLabel{"Sample Rate", &exportGroupBox};
    QComboBox exportSampleRateComboBox{&exportGroupBox};
    QLabel exportQuickExportDirLabel{"Always ask for quick export directory", &exportGroupBox};
    QCheckBox exportQuickExportDirCheckBox{&exportGroupBox};
    QLineEdit exportQuickExportDirLineEdit{&exportGroupBox};
    QPushButton exportQuickExportDirButton{"...", &exportGroupBox};

    QHBoxLayout botButtonLayout;
    QPushButton saveButton{"Save", this};
    QPushButton cancelButton{"Cancel", this};
};
