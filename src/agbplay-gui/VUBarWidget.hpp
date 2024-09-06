#pragma once

#include <QFrame>
#include <QLinearGradient>

class VUBarWidget : public QFrame
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

    void setMuted(bool muted);
    void setLevel(float rms, float peak);
    int offsetOfLevel(float level) const;

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

    QLinearGradient barGradientNormal;
    QLinearGradient barGradientMuted;
    QLinearGradient peakGradientNormal;
    QLinearGradient peakGradientMuted;
    int barLen = 0;

    float levelRms = 0.0f;
    float levelPeak = 0.0f;
    bool muted = false;
};
