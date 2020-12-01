#pragma once

#include <curses.h>
#include <cstdint>

class CursesWin {
public:
    CursesWin(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos);
    virtual ~CursesWin();
    virtual void Resize(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos);
protected:
    virtual void update();

    WINDOW *winPtr;
    uint32_t height, width;
};
