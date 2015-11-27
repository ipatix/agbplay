#include "PlaylistGUI.h"
#include "ColorDef.h"
#include <algorithm>

using namespace agbplay;

/*
 * public
 */

PlaylistGUI::PlaylistGUI(
        uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos) : SonglistGUI(
            height, width, yPos, xPos, false) {
    ticked = new vector<bool>;
    dragging = false;
    update();
}


PlaylistGUI::~PlaylistGUI() {
    delete ticked;
}

void PlaylistGUI::AddSong(SongEntry entry) {
    songlist->push_back(entry);
    ticked->push_back(true);
    update();
}

void PlaylistGUI::RemoveSong() {
    if (songlist->size() == 0) return;
    songlist->erase(songlist->begin() + cursorPos);
    ticked->erase(ticked->begin() + cursorPos);
    if (cursorPos != 0 && cursorPos >= songlist->size()) {
        cursorPos--;
    }
    update();
}

void PlaylistGUI::ClearSongs() {
    viewPos = 0;
    cursorPos = 0;
    songlist->clear();
    ticked->clear();
    update();
}

void PlaylistGUI::Tick() {
    if (ticked->size() == 0) return;
    ticked->at(cursorPos) = true;
    update();
}

void PlaylistGUI::Untick() {
    if (ticked->size() == 0) return;
    ticked->at(cursorPos) = false;
    update();
}

void PlaylistGUI::ToggleTick() {
    if (ticked->size() == 0) return;
    ticked->at(cursorPos) = !ticked->at(cursorPos);
    update();
}

void PlaylistGUI::ToggleDrag() {
    dragging = !dragging;
    update();
}

void PlaylistGUI::Leave() {
    dragging = false;
    SonglistGUI::Leave();
}

/*
 * private
 */

void PlaylistGUI::update() {
    UIMutex.lock();
    string bar = "Playlist:";
    bar.resize(contentWidth, ' ');
    wattrset(winPtr, COLOR_PAIR(GREEN_ON_DEFAULT) | A_REVERSE);
    mvwprintw(winPtr, 0, 0, bar.c_str());
    for (uint32_t i = 0; i < contentHeight; i++) {
        uint32_t entry = i + viewPos;
        if (entry == cursorPos && cursorVisible) {
            if (dragging) {
                wattrset(winPtr, COLOR_PAIR(RED_ON_DEFAULT) | A_REVERSE);
            } else {
                wattrset(winPtr, COLOR_PAIR(YELLOW_ON_DEFAULT) | A_REVERSE);
            }
        }
        else 
            wattrset(winPtr, COLOR_PAIR(YELLOW_ON_DEFAULT));
        string songText;
        if (entry < songlist->size()) {
            songText = (ticked->at(entry)) ? "[x] " : "[ ] ";
            songText += (*songlist)[entry].name;
        } else {
            songText = "";
        }
        songText.resize(width, ' ');
        mvwprintw(winPtr, (int)(height - contentHeight + (uint32_t)i), 0, songText.c_str());
    }
    wrefresh(winPtr);
    UIMutex.unlock();
}

void PlaylistGUI::scrollDownNoUpdate() {
    uint32_t pcursor = cursorPos;
    SonglistGUI::scrollDownNoUpdate();
    if (dragging && pcursor != cursorPos)
        swapEntry(pcursor, cursorPos);
}

void PlaylistGUI::scrollUpNoUpdate() { 
    uint32_t pcursor = cursorPos;
    SonglistGUI::scrollUpNoUpdate();
    if (dragging && pcursor != cursorPos)
        swapEntry(pcursor, cursorPos);
}

void PlaylistGUI::swapEntry(uint32_t a, uint32_t b) {
    size_t s = songlist->size();
    if (a >= s || b >= s)
        return;
    swap((*songlist)[a], (*songlist)[b]);
    swap((*ticked)[a], (*ticked)[b]);
    update();
}
