#pragma once

#include "CursesWin.h"

class VUMeterGUI : public CursesWin
{
public:
    VUMeterGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos);
    ~VUMeterGUI();
    void Resize(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos);
    void SetVol(float left, float right);
private:
    void update();

    float vuLevelLeft;
    float vuLevelRight;

    int meterWidth;
    int meterRed;
    int meterYel;
};
