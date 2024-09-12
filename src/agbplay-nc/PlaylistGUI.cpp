#include "PlaylistGUI.hpp"

#include "ColorDef.hpp"
#include "Debug.hpp"
#include "Util.hpp"
#include "Xcept.hpp"

#include <algorithm>
#include <cstring>

/*
 * public
 */

PlaylistGUI::PlaylistGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos) :
    SonglistGUI(height, width, yPos, xPos, false)
{
    // init
    ticked.resize(songlist.size(), true);
    update();
}

PlaylistGUI::~PlaylistGUI()
{
}

void PlaylistGUI::AddSong(const Profile::PlaylistEntry &entry)
{
    ticked.push_back(true);
    SonglistGUI::AddSong(entry);
}

void PlaylistGUI::RemoveSong()
{
    if (songlist.size() == 0)
        return;

    ticked.erase(ticked.begin() + cursorPos);
    SonglistGUI::RemoveSong();
}

void PlaylistGUI::ClearSongs()
{
    ticked.clear();
    SonglistGUI::ClearSongs();
}

const std::vector<bool> &PlaylistGUI::GetTicked() const
{
    return ticked;
}

void PlaylistGUI::Tick()
{
    if (ticked.size() == 0)
        return;
    ticked.at(cursorPos) = true;
    update();
}

void PlaylistGUI::Untick()
{
    if (ticked.size() == 0)
        return;
    ticked.at(cursorPos) = false;
    update();
}

void PlaylistGUI::ToggleTick()
{
    if (ticked.size() == 0)
        return;
    ticked.at(cursorPos) = !ticked.at(cursorPos);
    update();
}

void PlaylistGUI::ToggleDrag()
{
    dragging = !dragging;
    update();
}

void PlaylistGUI::UntickAll()
{
    fill(ticked.begin(), ticked.end(), false);
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
    std::string bar = "Playlist:";
    bar.resize(contentWidth, ' ');
    wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::WINDOW_FRAME)) | A_REVERSE);
    mvwprintw(winPtr, 0, 0, "%s", bar.c_str());
    for (uint32_t i = 0; i < contentHeight; i++) {
        uint32_t entry = i + viewPos;
        if (entry == cursorPos && cursorVisible) {
            if (dragging) {
                wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::LIST_SEL)) | A_REVERSE);
            } else {
                wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::LIST_ENTRY)) | A_REVERSE);
            }
        } else
            wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::LIST_ENTRY)));
        std::string songText;
        if (entry < songlist.size()) {
            songText = (ticked.at(entry)) ? "[x] " : "[ ] ";
            songText.append(songlist.at(entry).name);
        } else {
            songText = "";
        }
        songText.resize(width, ' ');
        mvwprintw(winPtr, (int)(height - contentHeight + (uint32_t)i), 0, "%s", songText.c_str());
    }
    wrefresh(winPtr);
}

void PlaylistGUI::scrollDownNoUpdate()
{
    uint32_t pcursor = cursorPos;
    if (cursorPos + 1 >= songlist.size())
        return;
    cursorPos++;
    if (viewPos + contentHeight < songlist.size() && cursorPos > viewPos + contentHeight - 5)
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
    if (a >= songlist.size() || b >= songlist.size())
        return;
    std::swap(songlist.at(a), songlist.at(b));
    std::vector<bool>::swap(ticked.at(a), ticked.at(b));
    update();
}
