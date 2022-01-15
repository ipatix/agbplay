#include "SonglistGUI.h"
#include "Xcept.h"
#include "ColorDef.h"
#include "Debug.h"
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

/*
 * -- public --
 */

SonglistGUI::SonglistGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos, bool upd) 
    : CursesWin(height, width, yPos, xPos) 
{
    checkDimensions(height, width);
    this->viewPos = 0;
    this->cursorPos = 0;
    this->cursorVisible = false;
    this->contentHeight = height - 1;
    this->contentWidth = width;
    if (upd) update();
}

SonglistGUI::~SonglistGUI() 
{
}

void SonglistGUI::Resize(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos) 
{
    checkDimensions(height, width);
    CursesWin::Resize(height, width, yPos, xPos);
    this->contentHeight = height - 1;
    this->contentWidth = width;
    update();
}

void SonglistGUI::Enter() 
{
    cursorVisible = true;
    update();
}

void SonglistGUI::Leave() 
{
    cursorVisible = false;
    update();
}

void SonglistGUI::ScrollDown() 
{
    scrollDownNoUpdate();
    update();
}

void SonglistGUI::ScrollUp() 
{
    scrollUpNoUpdate();
    update();
}

void SonglistGUI::PageDown() 
{
    for (uint32_t i = 0; i < contentHeight; i++) 
    {
        scrollDownNoUpdate();
    }
    update();
}

void SonglistGUI::PageUp() 
{
    for (uint32_t i = 0; i < contentHeight; i++) 
    {
        scrollUpNoUpdate();
    }
    update();
}

void SonglistGUI::AddSong(SongEntry entry) 
{
    songlist.push_back(entry);
    update();
}

void SonglistGUI::ClearSongs() 
{
    viewPos = 0;
    cursorPos = 0;
    songlist.clear();
    update();
}

void SonglistGUI::RemoveSong() 
{
    if (songlist.size() == 0) return;
    songlist.erase(songlist.begin() + cursorPos);
    if (cursorPos != 0 && cursorPos >= songlist.size()) {
        cursorPos--;
    }
    update();
}

SongEntry *SonglistGUI::GetSong()
{
    if (cursorPos >= songlist.size())
        return nullptr;
    return &songlist[cursorPos];
}

/*
 * -- private --
 */

void SonglistGUI::scrollDownNoUpdate() 
{
    // return if bounds are reached
    if (cursorPos + 1 >= songlist.size())
        return;
    cursorPos++;
    /* move viewport down of the cursor is in the lower area
     * and the screen may be scrolled down */
    if (viewPos + contentHeight < songlist.size() && cursorPos > viewPos + contentHeight - 5)
        viewPos++;
}

void SonglistGUI::scrollUpNoUpdate() 
{
    // return if bounds are reached
    if (cursorPos == 0)
        return;
    cursorPos--;
    if (viewPos > 0 && cursorPos < viewPos + 4)
        viewPos--;
}

void SonglistGUI::update() 
{
    wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::WINDOW_FRAME)) | A_REVERSE);
    mvwprintw(winPtr, 0, 0, "%-*.*s", contentWidth, contentWidth, "Songlist:");
    for (uint32_t i = 0; i < contentHeight; i++) {
        if (i + viewPos == cursorPos && cursorVisible)
            wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::LIST_ENTRY)) | A_REVERSE);
        else
            wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::LIST_ENTRY)));
        // generate list of songs
        if (i + viewPos < songlist.size()) {
            mvwprintw(winPtr, (int)(height - contentHeight + (uint32_t)i), 0, "%-*.*s", 
                    width, width, songlist[i + viewPos].name.c_str());
        } else {
            mvwprintw(winPtr, (int)(height - contentHeight + (uint32_t)i), 0, "%-*.*s", 
                    width, width, "");
        }
    }
    wrefresh(winPtr);
}

void SonglistGUI::checkDimensions(uint32_t height, uint32_t width) 
{
    if (height <= 1)
        throw Xcept("Songlist GUI height must not be 0");
    if (width < 5)
        throw Xcept("Songlist GUI width must be wider than 5");
}
