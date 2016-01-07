#include <thread>
#include <chrono>

#include "Events.h"

using namespace agbplay;

Events::Events() : CursesWin(1, 1, 0, 0)
{
    UIMutex.lock();
    keypad(winPtr, true);
    nodelay(winPtr, true);
    UIMutex.unlock();
}

Events::~Events()
{
}

int Events::GetKey()
{
    return wgetch(winPtr);
}

void Events::WaitTick()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
}
