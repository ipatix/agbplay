#pragma once

#include <QFrame>
#include <QGridLayout>
#include <QLabel>

#include "VUBarWidget.h"

class VUMeterWidget : public QFrame
{
    Q_OBJECT

public:
    VUMeterWidget(int width, int height, QWidget *parent = nullptr);
    ~VUMeterWidget() override;
    void SetLevel(float rmsLeft, float rmsRight, float peakLeft, float peakRight);

private:
    void paintEvent(QPaintEvent *paintEvent) override;

    QGridLayout layout{this};
    QLabel leftLabel{this};
    QLabel rightLabel{this};
    VUBarWidget leftBarWidget{VUBarWidget::Orientation::Right, true, -36.0f, 6.0f, this};
    VUBarWidget rightBarWidget{VUBarWidget::Orientation::Right, true, -36.0f, 6.0f, this};
};
