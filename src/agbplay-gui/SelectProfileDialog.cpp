#include "SelectProfileDialog.h"

#include <QListWidgetItem>
#include <QMessageBox>

#include <fmt/core.h>

#include "Profile.h"

SelectProfileDialog::SelectProfileDialog(QWidget *parent)
    : QDialog(parent)
{
    tableWidget.setMinimumSize(600, 150);
    mainLayout.addWidget(&tableWidget);
    mainLayout.addLayout(&buttonLayout, 0);
    buttonLayout.addStretch(1);
    buttonLayout.addWidget(&okButton);
    buttonLayout.addWidget(&cancelButton);
    buttonLayout.addStretch(1);

    tableWidget.setColumnCount(COL_COUNT);
    tableWidget.setHorizontalHeaderItem(COL_PATH, new QTableWidgetItem("Profile Path"));
    tableWidget.setHorizontalHeaderItem(COL_TABLE_POS, new QTableWidgetItem("Songtable Offset"));
    tableWidget.setHorizontalHeaderItem(COL_TABLE_INDEX, new QTableWidgetItem("Songtable Index"));
    tableWidget.setHorizontalHeaderItem(COL_DESCRIPTION, new QTableWidgetItem("Description"));
    tableWidget.setSelectionMode(QAbstractItemView::SingleSelection);
    tableWidget.setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(&cancelButton, &QPushButton::clicked, [this](bool){ done(QDialog::Rejected); });
    connect(&okButton, &QPushButton::clicked, this, &SelectProfileDialog::ok);
}

SelectProfileDialog::~SelectProfileDialog()
{
}

void SelectProfileDialog::addToSelectionDialog(const Profile &profile)
{
    QTableWidgetItem *pathItem = nullptr;
    QTableWidgetItem *tablePosItem = nullptr;
    QTableWidgetItem *tableIndexItem = nullptr;
    QTableWidgetItem *descriptionItem = nullptr;

    try {
        pathItem = new QTableWidgetItem();
        pathItem->setText(QString::fromStdWString(profile.path.wstring()));
        pathItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

        tablePosItem = new QTableWidgetItem();
        tablePosItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        tableIndexItem = new QTableWidgetItem();
        tableIndexItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

        if (profile.songTableInfoConfig.pos != SongTableInfo::POS_AUTO)
            tablePosItem->setText(QString::fromStdString(fmt::format("0x{:x}", profile.songTableInfoConfig.pos)));
        else
            tableIndexItem->setText(QString::number(profile.songTableInfoConfig.tableIdx));

        descriptionItem = new QTableWidgetItem();
        if (profile.description == "")
            descriptionItem->setText("<no description>");
        else
            descriptionItem->setText(QString::fromStdString(profile.description));
        descriptionItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

        const int row = tableWidget.rowCount();
        tableWidget.insertRow(row);
        tableWidget.setItem(row, COL_PATH, pathItem);
        tableWidget.setItem(row, COL_TABLE_POS, tablePosItem);
        tableWidget.setItem(row, COL_TABLE_INDEX, tableIndexItem);
        tableWidget.setItem(row, COL_DESCRIPTION, descriptionItem);
    } catch (...) {
        delete pathItem;
        delete tablePosItem;
        delete tableIndexItem;
        delete descriptionItem;
        throw;
    }
}

int SelectProfileDialog::selectedProfile() const
{
    return selected;
}

void SelectProfileDialog::ok(bool)
{
    selected = -1;

    auto l = tableWidget.selectedRanges();
    for (const auto &i : l) {
        if (selected == -1 && i.rowCount() == 1) {
            selected = i.topRow();
        } else if ((i.rowCount() == 1 && selected != i.topRow()) || i.rowCount() > 1) {
            QMessageBox mbox(QMessageBox::Icon::Critical, "Please select profile", "Only a single profile may be selected. Please try again.", QMessageBox::Ok, this);
            mbox.exec();
            return;
        }
    }

    if (selected < 0) {
        QMessageBox mbox(QMessageBox::Icon::Critical, "Please select profile", "No profile selected. Please try again.", QMessageBox::Ok, this);
        mbox.exec();
        return;
    }

    done(QDialog::Accepted);
}
