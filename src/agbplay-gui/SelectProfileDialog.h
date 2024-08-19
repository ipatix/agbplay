#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTableWidget>

struct Profile;

class SelectProfileDialog : public QDialog
{
    Q_OBJECT

public:
    SelectProfileDialog(QWidget *parent = nullptr);
    ~SelectProfileDialog() override;

    void addToSelectionDialog(const Profile &profile);
    int selectedProfile() const;

private:
    void ok(bool);

    QVBoxLayout mainLayout{this};
    QHBoxLayout buttonLayout;
    QTableWidget tableWidget{this};
    QPushButton okButton{"Ok", this};
    QPushButton cancelButton{"Cancel", this};

    int selected = -1;

    enum {
        COL_PATH,
        COL_TABLE_POS,
        COL_TABLE_INDEX,
        COL_DESCRIPTION,
        COL_COUNT,
    };
};
