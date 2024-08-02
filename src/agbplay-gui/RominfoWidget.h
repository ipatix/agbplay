#pragma once

#include <QGroupBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpacerItem>

class RominfoWidget : public QGroupBox
{
    Q_OBJECT

public:
    explicit RominfoWidget(QWidget *parent = nullptr);
    ~RominfoWidget() override;

    QVBoxLayout layout{this};

    QLabel romNameLabel{"ROM Name", this};
    QLineEdit romNameLineEdit{"<none>", this};
    QLabel romCodeLabel{"ROM Code", this};
    QLineEdit romCodeLineEdit{"<none>", this};
    QLabel songTableLabel{"Songtable Offset", this};
    QLineEdit songTableLineEdit{"<none>", this};
    QLabel songCountLabel{"Number of Songs:", this};
    QLineEdit songCountLineEdit{"<none>", this};

    QWidget spacer{this};
};
