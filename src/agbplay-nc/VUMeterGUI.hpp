#pragma once

#include "CursesWin.hpp"

class VUMeterGUI : public CursesWin
{
public:
    VUMeterGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos);
    ~VUMeterGUI() override;
    void Resize(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos) override;
    void SetVol(float left, float right);

private:
    void update() override;

    float vuLevelLeft;
    float vuLevelRight;

    int meterWidth;
    int meterRed;
    int meterYel;
};
