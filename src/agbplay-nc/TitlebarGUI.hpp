#pragma once

#include <cstdint>
#include "CursesWin.hpp"

class TitlebarGUI : public CursesWin {
public:
    TitlebarGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos);
    ~TitlebarGUI() override;
    void Resize(uint32_t height, uint32_t width,
            uint32_t yPos, uint32_t xPos) override;
    int GetKey();
private:
    void update() override;
};
