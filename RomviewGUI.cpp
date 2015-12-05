#include "RomviewGUI.h"
#include "ColorDef.h"

using namespace agbplay;
using namespace std;

/*
 * public
 */

RomviewGUI::RomviewGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos, Rom& rrom, SoundData& rsdata
        ) : CursesWin(height, width, yPos, xPos) {
    rrom.Seek(0xA0); // seek to game title;
    gameName = rrom.ReadString(12);
    gameCode = rrom.ReadString(4);
    songTable = rsdata.sTable->GetSongTablePos();
    numSongs = rsdata.sTable->GetNumSongs();
    update();
}

RomviewGUI::~RomviewGUI() {
}

void RomviewGUI::Resize(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos) {
    CursesWin::Resize(height, width, yPos, xPos);
    update();
}

/*
 * private
 */

void RomviewGUI::update() {
    UIMutex.lock();
    // clear
    wattrset(winPtr, A_NORMAL);
    wclear(winPtr);

    // draw borders
    wattrset(winPtr, COLOR_PAIR(Color::GRN_DEF) | A_REVERSE);
    mvwvline(winPtr, 1, 0, ' ', height-1);
    string title = " ROM Information:";
    title.resize(width, ' ');
    mvwprintw(winPtr, 0, 0, title.c_str());

    // print information
    wattrset(winPtr, A_UNDERLINE | COLOR_PAIR(Color::DEF_DEF));
    mvwprintw(winPtr, 2, 2, "ROM Name:");
    mvwprintw(winPtr, 3, 2, gameName.c_str());
    mvwprintw(winPtr, 5, 2, "ROM Code:");
    mvwprintw(winPtr, 6, 2, gameCode.c_str());
    mvwprintw(winPtr, 8, 2, "Songtable Offset:");
    char buf[32];
    snprintf(buf, sizeof(buf), "0x%lX", songTable);
    mvwprintw(winPtr, 9, 2, buf);
    mvwprintw(winPtr, 11, 2, "Song Amount:");
    mvwprintw(winPtr, 12, 2, to_string(numSongs).c_str());
    wrefresh(winPtr);
    UIMutex.unlock();
}
