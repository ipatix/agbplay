#include "SonglistGUI.h"
#include "MyException.h"
#include "ColorDef.h"
#include "Debug.h"
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

using namespace agbplay;
using namespace std;

/*
 * -- public --
 */

SonglistGUI::SonglistGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos, bool upd
        ) : CursesWin(height, width, yPos, xPos) {
    // init vars and create window
    checkDimensions(height, width);
    this->songlist = new vector<SongEntry>;
    this->viewPos = 0;
    this->cursorPos = 0;
    this->cursorVisible = false;
    this->contentHeight = height - 1;
    this->contentWidth = width;
    if (upd) update();
}

SonglistGUI::~SonglistGUI() {
    delete this->songlist;
}

void SonglistGUI::Resize(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos) {
    checkDimensions(height, width);
    CursesWin::Resize(height, width, yPos, xPos);
    this->contentHeight = height - 1;
    this->contentWidth = width;
    update();
}

void SonglistGUI::Enter() {
    cursorVisible = true;
    update();
}

void SonglistGUI::Leave() {
    cursorVisible = false;
    update();
}

void SonglistGUI::ScrollDown() {
    scrollDownNoUpdate();
    update();
}

void SonglistGUI::ScrollUp() {
    scrollUpNoUpdate();
    update();
}

void SonglistGUI::PageDown() {
    for (uint32_t i = 0; i < contentHeight; i++) {
        scrollDownNoUpdate();
    }
    update();
}

void SonglistGUI::PageUp() {
    for (uint32_t i = 0; i < contentHeight; i++) {
        scrollUpNoUpdate();
    }
    update();
}

void SonglistGUI::AddSong(SongEntry entry) {
    songlist->push_back(entry);
    update();
}

void SonglistGUI::ClearSongs() {
    viewPos = 0;
    cursorPos = 0;
    songlist->clear();
    update();
}

void SonglistGUI::RemoveSong() {
    if (songlist->size() == 0) return;
    songlist->erase(songlist->begin() + cursorPos);
    if (cursorPos != 0 && cursorPos >= songlist->size()) {
        cursorPos--;
    }
    update();
}

SongEntry SonglistGUI::GetSong() throw() {
    // will throw exception of out of bounds
    return songlist->at(cursorPos);
}

/*
 * -- private --
 */

void SonglistGUI::scrollDownNoUpdate() {
    // return if bounds are reached
    if (cursorPos + 1 >= songlist->size())
        return;
    cursorPos++;
    /* move viewport down of the cursor is in the lower area
     * and the screen may be scrolled down */
    if (viewPos + contentHeight < songlist->size() && cursorPos > viewPos + contentHeight - 5)
        viewPos++;
}

void SonglistGUI::scrollUpNoUpdate() {
    // return if bounds are reached
    if (cursorPos == 0)
        return;
    cursorPos--;
    if (viewPos > 0 && cursorPos < viewPos + 4)
        viewPos--;
}

void SonglistGUI::update() {
    string bar = "Songlist:";
    bar.resize(contentWidth, ' ');
    wattrset(winPtr, COLOR_PAIR(GREEN_ON_DEFAULT) | A_REVERSE);
    mvwprintw(winPtr, 0, 0, bar.c_str());
    for (uint32_t i = 0; i < contentHeight; i++) {
        if (i + viewPos == cursorPos && cursorVisible)
            wattrset(winPtr, COLOR_PAIR(YELLOW_ON_DEFAULT) | A_REVERSE);
        else
            wattrset(winPtr, COLOR_PAIR(YELLOW_ON_DEFAULT));
        // generate list of songs
        string songText;
        if (i + viewPos < songlist->size()) {
            songText = (*songlist)[i + viewPos].name;
        } else {
            songText = "";
        }
        songText.resize(width, ' ');
        mvwprintw(winPtr, (int)(height - contentHeight + (uint32_t)i), 0, songText.c_str());
    }
    wrefresh(winPtr);
}

void SonglistGUI::checkDimensions(uint32_t height, uint32_t width) {
    if (height <= 1)
        throw MyException("Songlist GUI height must not be 0");
    if (width < 5)
        throw MyException("Songlist GUI width must be wider than 5");
}

/*
 * SongEntry
 */

SongEntry::SongEntry(string name, uint32_t uid) {
    this->name = name;
    this->uid = uid;
}

SongEntry::~SongEntry() {
    // empty
}

uint32_t SongEntry::GetUID() {
    return uid;
}
