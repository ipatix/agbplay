#include "VUMeterWidget.h"

#include <QPainter>
#include <array>
#include <algorithm>

VUMeterWidget::VUMeterWidget(int width, int height, QWidget *parent)
    : QFrame(parent)
{
    setFixedSize(width, height);
    setFrameStyle(QFrame::Sunken | QFrame::Panel);
    setLineWidth(1);
}

VUMeterWidget::~VUMeterWidget()
{
}

void VUMeterWidget::SetLevel(float rmsLeft, float rmsRight, float peakLeft, float peakRight)
{
    this->rmsLeft = rmsLeft;
    this->rmsRight = rmsRight;
    this->peakLeft = peakLeft;
    this->peakRight = peakRight;
    update();
}

void VUMeterWidget::paintEvent(QPaintEvent *paintEvent)
{
    QPainter painter(this);

    const int LETTER_WIDTH = 12;
    const int DB_LEFT = -36;
    const int DB_RIGHT = 6;
    const std::array DB_POINTS = { -24, -12, 0 };

    const QRect bgRect(contentsRect());
    const QRect lLabelRect(
        bgRect.topLeft(),
        QSize(LETTER_WIDTH, bgRect.height()/2-2)
    );
    const QRect cHLineRect(
        lLabelRect.bottomLeft() + QPoint(0, 1),
        QSize(bgRect.width(), 2)
    );
    const QRect rLabelRect(
        cHLineRect.bottomLeft() + QPoint(0, 1),
        QPoint(LETTER_WIDTH, bgRect.bottom())
    );
    const QRect cVLineRect(
        lLabelRect.topRight() + QPoint(1, 0),
        rLabelRect.bottomRight() + QPoint(1, 0)
    );
    const QRect lVURect(
        lLabelRect.topRight() + QPoint(2, 0),
        QPoint(bgRect.right(), lLabelRect.bottom())
    );
    const QRect rVURect(
        rLabelRect.topRight() + QPoint(2, 0),
        QPoint(bgRect.right(), rLabelRect.bottom())
    );

    /* draw background */
    painter.fillRect(bgRect, QColor(10, 10, 10));
    painter.fillRect(cHLineRect, QColor(80, 80, 80));
    painter.fillRect(cVLineRect, QColor(80, 80, 80));

    /* draw L R letters */
    painter.setPen(QColor(255, 255, 255));
    painter.setFont(QFont("Monospace", 8));
    painter.drawText(lLabelRect, Qt::AlignCenter, "L");
    painter.drawText(rLabelRect, Qt::AlignCenter, "R");

    /* draw bars */
    const QLinearGradient barGradient(QPointF(0, 0), QPointF(width(), height()));
    const float dbRmsLeft = 20.0f * std::log10(std::max(0.0f, rmsLeft));
    const float dbRmsRight = 20.0f * std::log10(std::max(0.0f, rmsRight));
    const float dbPeakLeft = 20.0f * std::log10(std::max(0.0f, peakLeft));
    const float dbPeakRight = 20.0f * std::log10(std::max(0.0f, peakRight));

    auto calcWidth = [&](float x) {
        float tmp = (x - static_cast<float>(DB_LEFT)) / static_cast<float>(DB_RIGHT - DB_LEFT);
        tmp *= static_cast<float>(lVURect.width());
        return std::min(static_cast<int>(tmp), lVURect.width()-1);
    };

    auto drawBar = [&](const auto &rect, float rms) {
        const int widthRms = calcWidth(rms);
        if (widthRms < 0)
            return;

        const int widthGreen = calcWidth(-6.0f);
        const int widthRed = calcWidth(0.0f);
        QLinearGradient gradient(static_cast<float>(rect.left() + widthGreen), 0.0f, static_cast<float>(rect.left() + widthRed), 0.0f);
        gradient.setColorAt(0.0f, QColor(0, 200, 0));
        gradient.setColorAt(0.5f, QColor(200, 200, 0));
        gradient.setColorAt(1.0f, QColor(200, 0, 0));

        painter.fillRect(
            QRect(rect.topLeft(), QPoint(rect.left() + widthRms, rect.bottom())),
            gradient
        );
    };

    drawBar(lVURect, dbRmsLeft);
    drawBar(rVURect, dbRmsRight);

    auto drawPeak = [&, this](const auto &rect, float peak) {
        const int widthPeak = calcWidth(peak);
        if (widthPeak < 0)
            return;

        QColor col;

        if (peak < -6.0f) {
            col = QColor(0, 255, 0);
        } else if (peak < 0.0f) {
            const float t = std::clamp((peak + 6.0f) / 6.0f, 0.0f, 1.0f);
            col = QColor(static_cast<int>(t * 255.0f), static_cast<int>((1.0f - t) * 255.0f), 0);
        } else {
            col = QColor(255, 0, 0);
        }

        painter.fillRect(
            QRect(
                QPoint(rect.left() + widthPeak, rect.top()),
                QPoint(rect.left() + widthPeak, rect.bottom())
            ),
            col
        );
    };

    drawPeak(lVURect, dbPeakLeft);
    drawPeak(rVURect, dbPeakRight);

    /* draw border */
    QFrame::paintEvent(paintEvent);
}
