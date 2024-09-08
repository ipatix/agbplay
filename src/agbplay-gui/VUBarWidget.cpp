#include "VUBarWidget.hpp"

#include <QPainter>

VUBarWidget::VUBarWidget(Orientation orientation, bool logScale, float dbStart, float dbEnd, QWidget *parent) :
    QFrame(parent), orientation(orientation), logScale(logScale), dbStart(dbStart), dbEnd(dbEnd)
{
    barGradientNormal.setColorAt(0.0f, QColor(0, 200, 0));
    barGradientNormal.setColorAt(0.5f, QColor(200, 200, 0));
    barGradientNormal.setColorAt(1.0f, QColor(200, 0, 0));
    barGradientMuted.setColorAt(0.0f, QColor(180, 180, 180));
    barGradientMuted.setColorAt(1.0f, QColor(80, 80, 80));
    peakGradientNormal.setColorAt(0.0f, QColor(0, 255, 0));
    peakGradientNormal.setColorAt(0.5f, QColor(200, 255, 0));
    peakGradientNormal.setColorAt(1.0f, QColor(255, 0, 0));
    peakGradientMuted.setColorAt(0.0f, QColor(255, 255, 255));
    peakGradientMuted.setColorAt(1.0f, QColor(120, 120, 120));
}

VUBarWidget::~VUBarWidget()
{
}

void VUBarWidget::setMuted(bool muted)
{
    this->muted = muted;
    update();
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

    const QLinearGradient &barGradient = muted ? barGradientMuted : barGradientNormal;
    const QLinearGradient &peakGradient = muted ? peakGradientMuted : peakGradientNormal;

    int cr, cp;
    switch (orientation) {
    case Orientation::Left:
        cr = r.width() - 1 - offsetRms;
        cp = r.width() - 1 - offsetPeak;
        painter.fillRect(QRect(QPoint(cr, r.top()), r.bottomRight()), barGradient);
        painter.fillRect(QRect(QPoint(cp, r.top()), QPoint(cp, r.bottom())), peakGradient);
        break;
    case Orientation::Up:
        cr = r.height() - 1 - offsetRms;
        cp = r.height() - 1 - offsetPeak;
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
        barGradientNormal.setStart(barLen - offsetGreen, 0.0f);
        barGradientNormal.setFinalStop(barLen - offsetRed, 0.0f);
        barGradientMuted.setStart(barLen - offsetGreen, 0.0f);
        barGradientMuted.setFinalStop(barLen - offsetRed, 0.0f);
        peakGradientNormal.setStart(barLen - offsetGreen, 0.0f);
        peakGradientNormal.setFinalStop(barLen - offsetRed, 0.0f);
        peakGradientMuted.setStart(barLen - offsetGreen, 0.0f);
        peakGradientMuted.setFinalStop(barLen - offsetRed, 0.0f);
        break;
    case Orientation::Up:
        barGradientNormal.setStart(0.0f, barLen - offsetGreen);
        barGradientNormal.setFinalStop(0.0f, barLen - offsetRed);
        barGradientMuted.setStart(0.0f, barLen - offsetGreen);
        barGradientMuted.setFinalStop(0.0f, barLen - offsetRed);
        peakGradientNormal.setStart(0.0f, barLen - offsetGreen);
        peakGradientNormal.setFinalStop(0.0f, barLen - offsetRed);
        peakGradientMuted.setStart(0.0f, barLen - offsetGreen);
        peakGradientMuted.setFinalStop(0.0f, barLen - offsetRed);
        break;
    case Orientation::Right:
        barGradientNormal.setStart(offsetGreen, 0.0f);
        barGradientNormal.setFinalStop(offsetRed, 0.0f);
        barGradientMuted.setStart(offsetGreen, 0.0f);
        barGradientMuted.setFinalStop(offsetRed, 0.0f);
        peakGradientNormal.setStart(offsetGreen, 0.0f);
        peakGradientNormal.setFinalStop(offsetRed, 0.0f);
        peakGradientMuted.setStart(offsetGreen, 0.0f);
        peakGradientMuted.setFinalStop(offsetRed, 0.0f);
        break;
    case Orientation::Down:
        barGradientNormal.setStart(0.0f, offsetGreen);
        barGradientNormal.setFinalStop(0.0f, offsetRed);
        barGradientMuted.setStart(0.0f, offsetGreen);
        barGradientMuted.setFinalStop(0.0f, offsetRed);
        peakGradientNormal.setStart(0.0f, offsetGreen);
        peakGradientNormal.setFinalStop(0.0f, offsetRed);
        peakGradientMuted.setStart(0.0f, offsetGreen);
        peakGradientMuted.setFinalStop(0.0f, offsetRed);
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
