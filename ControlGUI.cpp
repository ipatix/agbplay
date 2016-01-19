#include "ControlGUI.h"

using namespace agbplay;

ControlGUI::ControlGUI() : CursesWin(1, 1, 0, 0)
{
    //UIMutex.lock();
    keypad(winPtr, true);
    nodelay(winPtr, true);
    //UIMutex.unlock();
}

ControlGUI::~ControlGUI()
{
}

int ControlGUI::GetKey()
{
    return wgetch(winPtr);
}
