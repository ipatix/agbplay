#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QTextEdit>

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
    QPushButton okButton{"Ok", this};
};
