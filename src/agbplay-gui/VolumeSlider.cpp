#include "VolumeSlider.hpp"

#include <array>
#include <QPainter>

VolumeSlider::VolumeSlider(QWidget *parent)
    : QSlider(parent)
{
}

VolumeSlider::~VolumeSlider()
{
}

void VolumeSlider::paintEvent(QPaintEvent *ev)
{
    const std::array<QPointF, 3> points = {{
        { 0.0, height() / 2.0 },
        { (qreal)width() - 1.0, (qreal)height() / 10.0 * 1.0 },
        { (qreal)width() - 1.0, (qreal)height() / 10.0 * 9.0 },
    }};

    QPainter painter{this};
    painter.setBrush(palette().brush(QPalette::Mid));
    painter.setPen(Qt::NoPen);
    painter.drawPolygon(points.data(), points.size());
    painter.setRenderHint(QPainter::Antialiasing);

    QSlider::paintEvent(ev);
}
