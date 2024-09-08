#pragma once

#include <QDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

class AboutWindow : public QDialog
{
    Q_OBJECT

public:
    AboutWindow(QWidget *parent = nullptr);
    ~AboutWindow() override;

private:
    QVBoxLayout layout{this};
    QHBoxLayout topLayout;
    QHBoxLayout botLayout;
    QLabel figletLabel{this};
    QLabel aboutInfoLabel{this};
    QPushButton okButton{"OK", this};
};
