#include "HotkeybarGUI.hpp"

#include "ColorDef.hpp"
#include "Xcept.hpp"

#include <cstdint>
#include <string>

HotkeybarGUI::HotkeybarGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos) :
    CursesWin(height, width, yPos, xPos)
{
    if (height == 0)
        throw Xcept("Hotkeybar can't be empty");
    update();
}

HotkeybarGUI::~HotkeybarGUI()
{
}

void HotkeybarGUI::update()
{
    // UIMutex.lock();
    wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::WINDOW_FRAME)) | A_REVERSE);
    // draw initial border
    mvwprintw(winPtr, 0, 0, "%-*s", width, " [q=QUIT] [tab=SWITCH] [a=ADD] [d=DEL] [g=DRAG]");

    for (uint32_t i = 1; i < height; i++) {
        mvwhline(winPtr, (int)i, 0, ' ', width);
    }
    wrefresh(winPtr);
    // UIMutex.unlock();
}

void HotkeybarGUI::Resize(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos)
{
    CursesWin::Resize(height, width, yPos, xPos);
    this->height = height;
    this->width = width;
    update();
}
