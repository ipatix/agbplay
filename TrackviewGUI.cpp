#include <boost/bind.hpp>

#include "TrackviewGUI.h"
#include "ColorDef.h"

using namespace std;
using namespace agbplay;

/*
 * public
 */

TrackviewGUI::TrackviewGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos
        ) : CursesWin(height, width, yPos, xPos) {
    // will clear the screen due to not overriding from CursesWin base class
    cursorPos = 0;
    cursorVisible = false;
    update();
}

TrackviewGUI::~TrackviewGUI() {
}

void TrackviewGUI::Resize(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos) {
    CursesWin::Resize(height, width, yPos, xPos);
    update();
}

void TrackviewGUI::SetState(DisplayContainer& disp) {
    this->disp = disp;
    update();
}

void TrackviewGUI::update() {
    UIMutex.lock();
    wclear(winPtr);
    const uint32_t yBias = 1;
    const uint32_t xBias = 1;

    // draw borderlines
    wattrset(winPtr, COLOR_PAIR(GREEN_ON_DEFAULT) | A_REVERSE);
    mvwvline(winPtr, 1, 0, ' ', height - 1);
    string titleText = " Tracker";
    titleText.resize(width, ' ');
    mvwprintw(winPtr, 0, 0, titleText.c_str());

    // draw track titlebar
    wattrset(winPtr, COLOR_PAIR(DEFAULT_ON_DEFAULT) | A_UNDERLINE);
    mvwprintw(winPtr, yBias, xBias, "[");
    wattrset(winPtr, COLOR_PAIR(GREEN_ON_DEFAULT) | A_UNDERLINE);
    wprintw(winPtr, "#t");
    wattrset(winPtr, COLOR_PAIR(DEFAULT_ON_DEFAULT) | A_UNDERLINE);
    wprintw(winPtr, "] ");
    wattrset(winPtr, COLOR_PAIR(GREEN_ON_DEFAULT) | A_UNDERLINE);
    wprintw(winPtr, "Location ");
    wattrset(winPtr, COLOR_PAIR(DEFAULT_ON_DEFAULT) | A_UNDERLINE);
    wprintw(winPtr, " ");
    wattrset(winPtr, COLOR_PAIR(RED_ON_DEFAULT) | A_UNDERLINE);
    wprintw(winPtr, "Del");
    wattrset(winPtr, COLOR_PAIR(DEFAULT_ON_DEFAULT) | A_UNDERLINE);
    wprintw(winPtr, " ");
    wattrset(winPtr, COLOR_PAIR(CYAN_ON_DEFAULT) | A_UNDERLINE);
    wprintw(winPtr, "Note");
    wattrset(winPtr, COLOR_PAIR(DEFAULT_ON_DEFAULT) | A_UNDERLINE);
    string blank = "";
    blank.resize(width - 24, ' '); // 24 makes the line fit to screen end
    wprintw(winPtr, blank.c_str());

    uint32_t th = 0;
    for (uint32_t i = 0; i < disp.data.size(); i++, th += 2) {
        // print tickbox
        unsigned long aFlag = (cursorVisible) ? A_REVERSE : 0;
        wattrset(winPtr, COLOR_PAIR(DEFAULT_ON_DEFAULT) | aFlag);
        mvwprintw(winPtr, (int)(yBias + 1 + th), xBias, "[");
        if (disp.data[i].isMuted) {
            wattrset(winPtr, COLOR_PAIR(RED_ON_DEFAULT) | aFlag);
        } else {
            wattrset(winPtr, COLOR_PAIR(GREEN_ON_DEFAULT) | aFlag);
        }
        wprintw(winPtr, "%2d", i);
        wattrset(winPtr, COLOR_PAIR(DEFAULT_ON_DEFAULT) | aFlag);
        wprintw(winPtr, "] ");
        wattrset(winPtr, COLOR_PAIR(GREEN_ON_DEFAULT) | aFlag);
        wprintw(winPtr, "0x%7X", disp.data[i].trackPtr);
        wattrset(winPtr, COLOR_PAIR(DEFAULT_ON_DEFAULT) | aFlag);
        wprintw(winPtr, " ");
        wattrset(winPtr, COLOR_PAIR(RED_ON_DEFAULT) | aFlag);
        wprintw(winPtr, "%2d ", disp.data[i].delay);
        wattrset(winPtr, COLOR_PAIR(DEFAULT_ON_DEFAULT) | aFlag);
        wprintw(winPtr, " ");
        wattrset(winPtr, COLOR_PAIR(CYAN_ON_DEFAULT) | aFlag);
        string notes = "";
        for (uint32_t j = 0; j < disp.data[i].activeNotes.size(); j++) {
            if (disp.data[i].activeNotes[j])
                notes += noteNames[j] + " ";
        }
        notes.resize(32, ' ');
        wprintw(winPtr, notes.c_str());
    }

    wrefresh(winPtr);
    UIMutex.unlock();
}
