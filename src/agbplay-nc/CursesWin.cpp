#include <cstdint>
#include <iostream>
#include <cstdlib>

#include "ColorDef.hpp"
#include "CursesWin.hpp"
#include "Xcept.hpp"

CursesWin::CursesWin(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos) {
    if ((winPtr = newwin((int)height, (int)width, (int)yPos, (int)xPos)) == nullptr) {
        throw Xcept("Error while creating curses window [newwin]");
    }
    this->height = height;
    this->width = width;
}

CursesWin::~CursesWin() {
    if (delwin(winPtr) == ERR) {
        //throw Xcept("Error while deleteing curses window [delwin]");
        std::cerr << "FATAL ERROR: deleting window of curses window failed" << std::endl;
        std::abort();
    }
}

void CursesWin::Resize(uint32_t height, uint32_t width,
        uint32_t yPos, uint32_t xPos) {
    if (delwin(winPtr) == ERR) {
        throw Xcept("Error while resizing curses window [delwin]");
    }
    this->height = height;
    this->width = width;
    if ((winPtr = newwin((int)height, (int)width, (int)yPos, (int)xPos)) == nullptr) {
        throw Xcept("Error while resizing curses window [newwin]");
    }
}

void CursesWin::update() {
    wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::DEF_DEF)));
    wclear(winPtr);
}
