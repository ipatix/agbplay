#include <cstdint>
#include "ColorDef.h"
#include "CursesWin.h"
#include "MyException.h"

using namespace agbplay;

//boost::mutex CursesWin::UIMutex;

CursesWin::CursesWin(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos) {
    //UIMutex.lock();
    if ((winPtr = newwin((int)height, (int)width, (int)yPos, (int)xPos)) == nullptr) {
        //UIMutex.unlock();
        throw MyException("Error while creating curses window [newwin]");
    }
    //UIMutex.unlock();
    this->height = height;
    this->width = width;
}

CursesWin::~CursesWin() {
    //UIMutex.lock();
    if (delwin(winPtr) == ERR) {
        //UIMutex.unlock();
        throw MyException("Error while deleteing curses window [delwin]");
    }
    //UIMutex.unlock();
}

void CursesWin::Resize(uint32_t height, uint32_t width,
        uint32_t yPos, uint32_t xPos) {
    //UIMutex.lock();
    if (delwin(winPtr) == ERR) {
        //UIMutex.unlock();
        throw MyException("Error while resizing curses window [delwin]");
    }
    this->height = height;
    this->width = width;
    if ((winPtr = newwin((int)height, (int)width, (int)yPos, (int)xPos)) == nullptr) {
        //UIMutex.unlock();
        throw MyException("Error while resizing curses window [newwin]");
    }
    //UIMutex.unlock();
}

void CursesWin::update() {
    //UIMutex.lock();
    wattrset(winPtr, COLOR_PAIR(Color::DEF_DEF));
    wclear(winPtr);
    //UIMutex.unlock();
}
