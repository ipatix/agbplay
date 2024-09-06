#include "VUMeterWidget.hpp"

#include <array>
#include <algorithm>

#include <QPainter>
#include <QPalette>

VUMeterWidget::VUMeterWidget(int width, int height, QWidget *parent)
    : QFrame(parent)
{
    setFixedSize(width, height);
    setFrameStyle(QFrame::Sunken | QFrame::Panel);
    setLineWidth(1);
    setStyleSheet("QFrame {background-color: #505050}");

    leftLabel.setText("L");
    leftLabel.setStyleSheet("QLabel {color: #FFFFFF; background-color: #0A0A0A}");
    leftLabel.setAlignment(Qt::AlignCenter);
    leftLabel.setFixedWidth(12);
    rightLabel.setText("R");
    rightLabel.setStyleSheet("QLabel {color: #FFFFFF; background-color: #0A0A0A}");
    rightLabel.setAlignment(Qt::AlignCenter);
    rightLabel.setFixedWidth(12);

    leftBarWidget.setStyleSheet("QFrame {background-color: #0A0A0A}");
    rightBarWidget.setStyleSheet("QFrame {background-color: #0A0A0A}");

    layout.setHorizontalSpacing(0);
    layout.setVerticalSpacing(0);
    layout.setContentsMargins(0, 0, 0, 0);
    layout.addWidget(&leftLabel, 0, 0);
    layout.addWidget(&rightLabel, 2, 0);
    layout.addWidget(&leftBarWidget, 0, 2);
    layout.addWidget(&rightBarWidget, 2, 2);
    layout.setColumnMinimumWidth(1, 2);
    layout.setColumnStretch(1, 0);
    layout.setRowMinimumHeight(1, 2);
    layout.setRowStretch(1, 0);
}

VUMeterWidget::~VUMeterWidget()
{
}

void VUMeterWidget::SetLevel(float rmsLeft, float rmsRight, float peakLeft, float peakRight)
{
    leftBarWidget.setLevel(rmsLeft, peakLeft);
    rightBarWidget.setLevel(rmsRight, peakRight);
    update();
}

void VUMeterWidget::paintEvent(QPaintEvent *paintEvent)
{
    QPainter painter(this);

    const std::array DB_POINTS = { -24, -12, 0 };

    painter.fillRect(QRect(0, 0, 4, 4), QColor(255, 0, 255));

    /* draw scale */
    //for (auto point : DB_POINTS) {
    //    QRect lVURect = leftBarWidget.contentsRect();
    //    QRect rVURect = rightBarWidget.contentsRect();
    //    const int x = leftBarWidget.offsetOfLevel(static_cast<float>(point));
    //    const int lx = x + lVURect.left();
    //    const int rx = x + rVURect.left();
    //    const int lyt = lVURect.top();
    //    const int lyb = lVURect.top() + lVURect.height() / 3;
    //    const int ryt = rVURect.bottom() - rVURect.height() / 3;
    //    const int ryb = rVURect.bottom();
    //    painter.fillRect(QRect(QPoint(lx, lyt), QPoint(lx, lyb)), QColor(255, 255, 255));
    //    //painter.fillRect(QRect(QPoint(rx, ryt), QPoint(rx, ryb)), QColor(255, 255, 255));

    //    const QRect labelRect(lx - 8, height() / 2 - 8, 16, 16);
    //    painter.setPen(QColor(255, 255, 255));
    //    QFont font;
    //    font.setPointSize(7);
    //    painter.setFont(font);
    //    //painter.drawText(labelRect, Qt::AlignCenter, QString::number(point));
    //}

    /* draw border */
    QFrame::paintEvent(paintEvent);
}
