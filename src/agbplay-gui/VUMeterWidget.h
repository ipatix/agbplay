#pragma once

#include <QWidget>

class VUMeterWidget : public QWidget
{
    Q_OBJECT

public:
    VUMeterWidget(int width, int height, QWidget *parent = nullptr);
    ~VUMeterWidget() override;
    void SetLevel(float rmsLeft, float rmsRight, float peakLeft, float peakRight);

private:
    void paintEvent(QPaintEvent *paintEvent) override;

    float rmsLeft = 0.0f;
    float rmsRight = 0.0f;
    float peakLeft = 0.0f;
    float peakRight = 0.0f;
};
