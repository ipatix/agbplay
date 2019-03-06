#include <algorithm>
#include <cstring>

#include "PlaylistGUI.h"
#include "ColorDef.h"
#include "Util.h"
#include "Xcept.h"
#include "Debug.h"
#include "ConfigManager.h"

using namespace agbplay;
using namespace std;

/*
 * public
 */

PlaylistGUI::PlaylistGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos) 
    : SonglistGUI(height, width, yPos, xPos, false)
{
    // init
    ticked = new vector<bool>(ConfigManager::Instance().GetCfg().GetGameEntries().size(), true);
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
    ConfigManager::Instance().GetCfg().GetGameEntries().push_back(entry);
    ticked->push_back(true);
    update();
}

void PlaylistGUI::RemoveSong() 
{
    GameConfig& cfg = ConfigManager::Instance().GetCfg();

    if (cfg.GetGameEntries().size() == 0)
        return;

    cfg.GetGameEntries().erase(cfg.GetGameEntries().begin() + cursorPos);
    ticked->erase(ticked->begin() + cursorPos);

    if (cursorPos != 0 && cursorPos >= cfg.GetGameEntries().size()) {
        cursorPos--;
    }

    update();
}

void PlaylistGUI::ClearSongs() 
{
    viewPos = 0;
    cursorPos = 0;
    GameConfig& cfg = ConfigManager::Instance().GetCfg();
    cfg.GetGameEntries().clear();
    ticked->clear();
    update();
}

SongEntry& PlaylistGUI::GetSong()
{
    GameConfig& cfg = ConfigManager::Instance().GetCfg();
    return cfg.GetGameEntries().at(cursorPos);
}

vector<bool>& PlaylistGUI::GetTicked()
{
    return *ticked;
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

void PlaylistGUI::UntickAll()
{
    fill(ticked->begin(), ticked->end(), false);
    update();
}

bool PlaylistGUI::IsDragging()
{
    return dragging;
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
    GameConfig& cfg = ConfigManager::Instance().GetCfg();
    string bar = "Playlist:";
    bar.resize(contentWidth, ' ');
    wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::WINDOW_FRAME)) | A_REVERSE);
    mvwprintw(winPtr, 0, 0, bar.c_str());
    for (uint32_t i = 0; i < contentHeight; i++) {
        uint32_t entry = i + viewPos;
        if (entry == cursorPos && cursorVisible) {
            if (dragging) {
                wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::LIST_SEL)) | A_REVERSE);
            } else {
                wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::LIST_ENTRY)) | A_REVERSE);
            }
        }
        else 
            wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::LIST_ENTRY)));
        string songText;
        if (entry < cfg.GetGameEntries().size()) {
            songText = (ticked->at(entry)) ? "[x] " : "[ ] ";
            songText.append(cfg.GetGameEntries()[entry].name);
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
    GameConfig& cfg = ConfigManager::Instance().GetCfg();
    uint32_t pcursor = cursorPos;
    if (cursorPos + 1 >= cfg.GetGameEntries().size())
        return;
    cursorPos++;
    if (viewPos + contentHeight < cfg.GetGameEntries().size() && cursorPos > viewPos + contentHeight - 5)
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
    GameConfig& cfg = ConfigManager::Instance().GetCfg();
    size_t s = cfg.GetGameEntries().size();
    if (a >= s || b >= s)
        return;
    swap(cfg.GetGameEntries()[a], cfg.GetGameEntries()[b]);
    swap((*ticked)[a], (*ticked)[b]);
    update();
}
