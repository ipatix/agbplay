#pragma once

#include <cstdint>
#include <curses.h>

class CursesWin
{
public:
    CursesWin(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos);
    CursesWin(const CursesWin &) = delete;
    CursesWin &operator=(const CursesWin &) = delete;
    virtual ~CursesWin();
    virtual void Resize(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos);

protected:
    virtual void update();

    WINDOW *winPtr;
    uint32_t height, width;
};
