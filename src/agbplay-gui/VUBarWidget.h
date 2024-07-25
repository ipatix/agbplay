#pragma once

#include <QWidget>
#include <QLinearGradient>

class VUBarWidget : public QWidget
{
    Q_OBJECT

public:
    enum class Orientation {
        Left,
        Up,
        Right,
        Down,
    };

    VUBarWidget(Orientation orientation, bool logScale, float dbStart = -36.0f, float dbEnd = 6.0f, QWidget *parent = nullptr);
    ~VUBarWidget() override;

    void setLevel(float rms, float peak);

private:
    void paintEvent(QPaintEvent *paintEvent) override;
    void resizeEvent(QResizeEvent *resizeEvent) override;
    void updateGradient();
    int offsetOfRms() const;
    int offsetOfPeak() const;
    int offsetOfLog(float level) const;
    int offsetOfLinear(float level) const;

    Orientation orientation;
    const bool logScale;
    const float dbStart;
    const float dbEnd;

    QLinearGradient barGradient;
    QLinearGradient peakGradient;
    int barLen = 0;

    float levelRms = 0.0f;
    float levelPeak = 0.0f;
};
