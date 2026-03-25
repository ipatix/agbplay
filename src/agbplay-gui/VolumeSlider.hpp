#pragma once

#include <QSlider>

class VolumeSlider : public QSlider
{
public:
    VolumeSlider(QWidget *parent = nullptr);
    virtual ~VolumeSlider();

protected:
    virtual void paintEvent(QPaintEvent *ev) override;
};
