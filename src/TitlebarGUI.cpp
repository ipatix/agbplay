#include <cstring>
#include <string>
#include <vector>
#include "TitlebarGUI.h"
#include "ColorDef.h"
#include "MyException.h"

using namespace agbplay;
using namespace std;

static const vector<string> bannerText = {
    "            __        __         ",
    " ___ ____ _/ /  ___  / /__ ___ __",
    "/ _ `/ _ `/ _ \\/ _ \\/ / _ `/ // /",
    "\\_,_/\\_, /_.__/ .__/_/\\_,_/\\_, / ",
    "    /___/    /_/          /___/  " 
};

TitlebarGUI::TitlebarGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos) 
    : CursesWin(height, width, yPos, xPos) 
{
    if (width < bannerText[0].size())
        throw MyException("Terminal too narrow for banner text");
    if (height < bannerText.size())
        throw MyException("Terminal to flat for banner text");
    keypad(winPtr, true);
    nodelay(winPtr, true);
    update();
}

TitlebarGUI::~TitlebarGUI() 
{
}

void TitlebarGUI::Resize(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos) 
{
    CursesWin::Resize(height, width, yPos, xPos);
    keypad(winPtr, true);
    nodelay(winPtr, true);
    update();
}

int TitlebarGUI::GetKey()
{
    return wgetch(winPtr);
}

/*
 * private TitlebarGUI
 */

void TitlebarGUI::update() 
{
    uint32_t textHeight = uint32_t(bannerText.size());
    uint32_t upperPadding = uint32_t((height - textHeight) / 2);
    // unused for now
    for (uint32_t i = 0; i < height; i++) 
    {
        // print upper padding
        string tmp;
        if (i < upperPadding || i > height - upperPadding) {
            tmp = "";
            tmp.resize(width, ' ');
            wattrset(winPtr, COLOR_PAIR(Color::CYN_DEF) | A_REVERSE);
            mvwprintw(winPtr, (int)i, 0, "%s", tmp.c_str());
        } else {
            uint32_t leftPadding = (uint32_t)((width - bannerText[i - upperPadding].size()) / 2);
            uint32_t rightPadding = (uint32_t)((width - bannerText[i - upperPadding].size()) / 2 +
                ((width - bannerText[i - upperPadding].size()) % 2));
            tmp = "";
            tmp.resize(leftPadding, ' ');
            wattrset(winPtr, COLOR_PAIR(Color::CYN_DEF) | A_REVERSE);
            mvwprintw(winPtr, (int)i, 0, "%s", tmp.c_str());
            tmp = "";
            tmp.resize(rightPadding, ' ');
            wattrset(winPtr, COLOR_PAIR(Color::CYN_DEF));
            wprintw(winPtr, "%s", bannerText[i - upperPadding].c_str());
            wattrset(winPtr, COLOR_PAIR(Color::CYN_DEF) | A_REVERSE);
            wprintw(winPtr, tmp.c_str());
        }
    }
    wrefresh(winPtr);
}
