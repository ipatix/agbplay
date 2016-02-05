#include <algorithm>
#include <cstring>

#include "PlaylistGUI.h"
#include "ColorDef.h"
#include "Util.h"
#include "MyException.h"
#include "Debug.h"

using namespace agbplay;
using namespace std;

/*
 * public
 */

PlaylistGUI::PlaylistGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos, GameConfig& _thisGameConfig) 
    : SonglistGUI(height, width, yPos, xPos, false), thisGameConfig(_thisGameConfig)
{
    // init
    ticked = new vector<bool>(_thisGameConfig.GetGameEntries().size(), true);
    this->gameCode = gameCode;
    dragging = false;
    update();
}


PlaylistGUI::~PlaylistGUI() 
{
    delete ticked;
}

void PlaylistGUI::AddSong(SongEntry entry) 
{
    thisGameConfig.GetGameEntries().push_back(entry);
    ticked->push_back(true);
    update();
}

void PlaylistGUI::RemoveSong() 
{
    if (thisGameConfig.GetGameEntries().size() == 0) return;
    thisGameConfig.GetGameEntries().erase(thisGameConfig.GetGameEntries().begin() + cursorPos);
    ticked->erase(ticked->begin() + cursorPos);
    if (cursorPos != 0 && cursorPos >= thisGameConfig.GetGameEntries().size()) {
        cursorPos--;
    }
    update();
}

void PlaylistGUI::ClearSongs() 
{
    viewPos = 0;
    cursorPos = 0;
    thisGameConfig.GetGameEntries().clear();
    ticked->clear();
    update();
}

SongEntry& PlaylistGUI::GetSong()
{
    return thisGameConfig.GetGameEntries().at(cursorPos);
}

void PlaylistGUI::Tick() 
{
    if (ticked->size() == 0) return;
    ticked->at(cursorPos) = true;
    update();
}

void PlaylistGUI::Untick() 
{
    if (ticked->size() == 0) return;
    ticked->at(cursorPos) = false;
    update();
}

void PlaylistGUI::ToggleTick() 
{
    if (ticked->size() == 0) return;
    ticked->at(cursorPos) = !ticked->at(cursorPos);
    update();
}

void PlaylistGUI::ToggleDrag() 
{
    dragging = !dragging;
    update();
}

void PlaylistGUI::Leave() 
{
    dragging = false;
    SonglistGUI::Leave();
}

/*
 * private
 */

void PlaylistGUI::update() 
{
    //UIMutex.lock();
    string bar = "Playlist:";
    bar.resize(contentWidth, ' ');
    wattrset(winPtr, COLOR_PAIR(Color::GRN_DEF) | A_REVERSE);
    mvwprintw(winPtr, 0, 0, bar.c_str());
    for (uint32_t i = 0; i < contentHeight; i++) {
        uint32_t entry = i + viewPos;
        if (entry == cursorPos && cursorVisible) {
            if (dragging) {
                wattrset(winPtr, COLOR_PAIR(Color::RED_DEF) | A_REVERSE);
            } else {
                wattrset(winPtr, COLOR_PAIR(Color::YEL_DEF) | A_REVERSE);
            }
        }
        else 
            wattrset(winPtr, COLOR_PAIR(Color::YEL_DEF));
        string songText;
        if (entry < thisGameConfig.GetGameEntries().size()) {
            songText = (ticked->at(entry)) ? "[x] " : "[ ] ";
            songText.append(thisGameConfig.GetGameEntries()[entry].name);
        } else {
            songText = "";
        }
        songText.resize(width, ' ');
        mvwprintw(winPtr, (int)(height - contentHeight + (uint32_t)i), 0, songText.c_str());
    }
    wrefresh(winPtr);
    //UIMutex.unlock();
}

void PlaylistGUI::scrollDownNoUpdate() 
{
    uint32_t pcursor = cursorPos;
    if (cursorPos + 1 >= thisGameConfig.GetGameEntries().size())
        return;
    cursorPos++;
    if (viewPos + contentHeight < thisGameConfig.GetGameEntries().size() && cursorPos > viewPos + contentHeight - 5)
        viewPos++;
    if (dragging && pcursor != cursorPos)
        swapEntry(pcursor, cursorPos);
}

void PlaylistGUI::scrollUpNoUpdate() 
{
    uint32_t pcursor = cursorPos;
    if (cursorPos == 0)
        return;
    cursorPos--;
    if (viewPos > 0 && cursorPos < viewPos + 4)
        viewPos--;
    if (dragging && pcursor != cursorPos)
        swapEntry(pcursor, cursorPos);
}

void PlaylistGUI::swapEntry(uint32_t a, uint32_t b) 
{
    size_t s = thisGameConfig.GetGameEntries().size();
    if (a >= s || b >= s)
        return;
    swap(thisGameConfig.GetGameEntries()[a], thisGameConfig.GetGameEntries()[b]);
    swap((*ticked)[a], (*ticked)[b]);
    update();
}
