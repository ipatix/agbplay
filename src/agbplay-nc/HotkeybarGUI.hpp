#pragma once

#include "CursesWin.hpp"

class HotkeybarGUI : public CursesWin {
public:
    HotkeybarGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos);
    ~HotkeybarGUI() override;

    void Resize(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos) override;

private:
    void update() override;
};
