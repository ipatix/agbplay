#include "VUBarWidget.h"

#include <QPainter>

VUBarWidget::VUBarWidget(Orientation orientation, bool logScale, float dbStart, float dbEnd, QWidget *parent)
    : QFrame(parent), orientation(orientation), logScale(logScale), dbStart(dbStart), dbEnd(dbEnd)
{
    barGradient.setColorAt(0.0f, QColor(0, 200, 0));
    barGradient.setColorAt(0.5f, QColor(200, 200, 0));
    barGradient.setColorAt(1.0f, QColor(200, 0, 0));
    peakGradient.setColorAt(0.0f, QColor(0, 255, 0));
    peakGradient.setColorAt(0.5f, QColor(200, 255, 0));
    peakGradient.setColorAt(1.0f, QColor(255, 0, 0));
}

VUBarWidget::~VUBarWidget()
{
}

void VUBarWidget::setLevel(float rms, float peak)
{
    levelRms = rms;
    levelPeak = peak;
    update();
}

int VUBarWidget::offsetOfLevel(float level) const
{
    if (logScale)
        return offsetOfLog(level);
    return offsetOfLinear(level);
}

void VUBarWidget::paintEvent(QPaintEvent *paintEvent)
{
    int offsetRms, offsetPeak;
    if (logScale) {
        offsetRms = offsetOfLog(levelRms);
        offsetPeak = offsetOfLog(levelPeak);
    } else {
        offsetRms = offsetOfLinear(levelRms);
        offsetPeak = offsetOfLinear(levelPeak);
    }

    QPainter painter(this);

    QRect r = contentsRect();

    int cr, cp;
    switch (orientation) {
    case Orientation::Left:
        cr = width() - offsetRms;
        cp = width() - offsetPeak;
        painter.fillRect(QRect(QPoint(cr, r.top()), r.bottomRight()), barGradient);
        painter.fillRect(QRect(QPoint(cp, r.top()), QPoint(cp, r.bottom())), peakGradient);
        break;
    case Orientation::Up:
        cr = height() - offsetRms;
        cp = height() - offsetPeak;
        painter.fillRect(QRect(QPoint(r.left(), cr), r.bottomRight()), barGradient);
        painter.fillRect(QRect(QPoint(r.left(), cp), QPoint(r.right(), cp)), peakGradient);
        break;
    case Orientation::Right:
        cr = offsetRms;
        cp = offsetPeak;
        painter.fillRect(QRect(r.topLeft(), QPoint(cr, r.bottom())), barGradient);
        painter.fillRect(QRect(QPoint(cp, r.top()), QPoint(cp, r.bottom())), peakGradient);
        break;
    case Orientation::Down:
        cr = offsetRms;
        cp = offsetPeak;
        painter.fillRect(QRect(r.topLeft(), QPoint(r.right(), cr)), barGradient);
        painter.fillRect(QRect(QPoint(r.left(), cp), QPoint(r.right(), cp)), peakGradient);
        break;
    }

    QFrame::paintEvent(paintEvent);
}

void VUBarWidget::resizeEvent(QResizeEvent *resizeEvent)
{
    (void)resizeEvent;

    if (orientation == Orientation::Left || orientation == Orientation::Right)
        barLen = width();
    else
        barLen = height();
    updateGradient();
}

void VUBarWidget::updateGradient()
{
    int offsetGreen, offsetRed;
    if (logScale) {
        offsetGreen = offsetOfLog(0.5);
        offsetRed = offsetOfLog(1.0);
    } else {
        offsetGreen = offsetOfLinear(0.5);
        offsetRed = offsetOfLinear(1.0);
    }

    switch (orientation) {
    case Orientation::Left:
        barGradient.setStart(barLen - offsetGreen, 0.0f);
        barGradient.setFinalStop(barLen - offsetRed, 0.0f);
        peakGradient.setStart(barLen - offsetGreen, 0.0f);
        peakGradient.setFinalStop(barLen - offsetRed, 0.0f);
        break;
    case Orientation::Up:
        barGradient.setStart(0.0f, barLen - offsetGreen);
        barGradient.setFinalStop(0.0f, barLen - offsetRed);
        peakGradient.setStart(0.0f, barLen - offsetGreen);
        peakGradient.setFinalStop(0.0f, barLen - offsetRed);
        break;
    case Orientation::Right:
        barGradient.setStart(offsetGreen, 0.0f);
        barGradient.setFinalStop(offsetRed, 0.0f);
        peakGradient.setStart(offsetGreen, 0.0f);
        peakGradient.setFinalStop(offsetRed, 0.0f);
        break;
    case Orientation::Down:
        barGradient.setStart(0.0f, offsetGreen);
        barGradient.setFinalStop(0.0f, offsetRed);
        peakGradient.setStart(0.0f, offsetGreen);
        peakGradient.setFinalStop(0.0f, offsetRed);
        break;
    }
}

int VUBarWidget::offsetOfRms() const
{
    if (logScale)
        return offsetOfLog(levelRms);
    return offsetOfLinear(levelRms);
}

int VUBarWidget::offsetOfPeak() const
{
    if (logScale)
        return offsetOfLog(levelPeak);
    return offsetOfLinear(levelPeak);
}

int VUBarWidget::offsetOfLog(float level) const
{
    const float db = 20 * std::log10(std::max(0.0f, level));
    float tmp = (db - dbStart) / (dbEnd - dbStart);
    tmp *= static_cast<float>(barLen);
    return std::clamp(static_cast<int>(tmp), 0, barLen); 
}

int VUBarWidget::offsetOfLinear(float level) const
{
    return std::clamp(static_cast<int>(level * static_cast<float>(barLen)), 0, barLen);
}
