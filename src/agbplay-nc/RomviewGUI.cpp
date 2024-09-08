#include "RomviewGUI.hpp"

#include "ColorDef.hpp"
#include "Debug.hpp"
#include "Rom.hpp"
#include "Util.hpp"

/*
 * public
 */

RomviewGUI::RomviewGUI(
    uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos, const SongTableInfo &songTableInfo
) :
    CursesWin(height, width, yPos, xPos), songTableInfo(songTableInfo)
{
    gameName = Rom::Instance().ReadString(0xA0, 12);
    gameCode = Rom::Instance().GetROMCode();
    update();
}

RomviewGUI::~RomviewGUI()
{
}

void RomviewGUI::Resize(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos)
{
    CursesWin::Resize(height, width, yPos, xPos);
    update();
}

/*
 * private
 */

void RomviewGUI::update()
{
    // clear
    wattrset(winPtr, A_NORMAL);
    wclear(winPtr);

    // draw borders
    wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::WINDOW_FRAME)) | A_REVERSE);
    mvwvline(winPtr, 1, 0, ' ', height - 1);
    mvwprintw(winPtr, 0, 0, "%-*s", width, " ROM Information");

    // print information
    wattrset(winPtr, A_UNDERLINE | COLOR_PAIR(static_cast<int>(Color::DEF_DEF)));
    mvwprintw(winPtr, 2, 2, "ROM Name:");
    wattrset(winPtr, A_BOLD | COLOR_PAIR(static_cast<int>(Color::DEF_DEF)));
    mvwprintw(winPtr, 3, 2, "%s", gameName.c_str());
    wattrset(winPtr, A_UNDERLINE | COLOR_PAIR(static_cast<int>(Color::DEF_DEF)));
    mvwprintw(winPtr, 5, 2, "ROM Code:");
    wattrset(winPtr, A_BOLD | COLOR_PAIR(static_cast<int>(Color::DEF_DEF)));
    mvwprintw(winPtr, 6, 2, "%s", gameCode.c_str());
    wattrset(winPtr, A_UNDERLINE | COLOR_PAIR(static_cast<int>(Color::DEF_DEF)));
    mvwprintw(winPtr, 8, 2, "Songtable Offset:");
    wattrset(winPtr, A_BOLD | COLOR_PAIR(static_cast<int>(Color::DEF_DEF)));
    mvwprintw(winPtr, 9, 2, "0x%lX", songTableInfo.pos);
    wattrset(winPtr, A_UNDERLINE | COLOR_PAIR(static_cast<int>(Color::DEF_DEF)));
    mvwprintw(winPtr, 11, 2, "Number of Songs:");
    wattrset(winPtr, A_BOLD | COLOR_PAIR(static_cast<int>(Color::DEF_DEF)));
    mvwprintw(winPtr, 12, 2, "%zu", static_cast<size_t>(songTableInfo.count));
    wrefresh(winPtr);
}
