#include <cstring>
#include <string>
#include "TitlebarGUI.h"
#include "ColorDef.h"
#include "MyException.h"

using namespace agbplay;

static const char *bannerText[] = {
    "            __        __         ",
    " ___ ____ _/ /  ___  / /__ ___ __",
    "/ _ `/ _ `/ _ \\/ _ \\/ / _ `/ // /",
    "\\_,_/\\_, /_.__/ .__/_/\\_,_/\\_, / ",
    "    /___/    /_/          /___/  " 
};

TitlebarGUI::TitlebarGUI(uint32_t height, uint32_t width, uint32_t yPos, 
        uint32_t xPos) : CursesWin(height, width, yPos, xPos) {
    // FIXME, maybe check all lines
    if (width < strlen(bannerText[0]))
        throw MyException("Terminal too narrow for banner text");
    if (height < sizeof(bannerText) / sizeof(char *))
        throw MyException("Terminal to flat for banner text");
    update();
}

TitlebarGUI::~TitlebarGUI() {
}

void TitlebarGUI::Resize(uint32_t height, uint32_t width, 
        uint32_t yPos, uint32_t xPos) {
    CursesWin::Resize(height, width, yPos, xPos);
    update();
}

void TitlebarGUI::update() {
    uint32_t textHeight = sizeof(bannerText) / sizeof(char *);
    uint32_t upperPadding = (uint32_t)((height - textHeight) / 2);
    // unused for now
    // uint32_t lowerPadding = (uint32_t)((height - textHeight) / 2 + ((height - textHeight) % 2));
    for (uint32_t i = 0; i < height; i++) {
        // print upper padding
        std::string tmp;
        if (i < upperPadding || i > height - upperPadding) {
            tmp = "";
            tmp.resize(width, ' ');
            wattrset(winPtr, COLOR_PAIR(CYAN_ON_DEFAULT) | A_REVERSE);
            mvwprintw(winPtr, (int)i, 0, tmp.c_str());
        } else {
            uint32_t leftPadding = (uint32_t)((width - strlen(bannerText[i - upperPadding])) / 2);
            uint32_t rightPadding = (uint32_t)((width - strlen(bannerText[i - upperPadding])) / 2 +
                ((width - strlen(bannerText[i - upperPadding])) % 2));
            tmp = "";
            tmp.resize(leftPadding, ' ');
            wattrset(winPtr, COLOR_PAIR(CYAN_ON_DEFAULT) | A_REVERSE);
            mvwprintw(winPtr, (int)i, 0, tmp.c_str());
            tmp = "";
            tmp.resize(rightPadding, ' ');
            wattrset(winPtr, COLOR_PAIR(CYAN_ON_DEFAULT));
            wprintw(winPtr, "%s", bannerText[i - upperPadding]);
            wattrset(winPtr, COLOR_PAIR(CYAN_ON_DEFAULT) | A_REVERSE);
            wprintw(winPtr, tmp.c_str());
        }
    }
    wrefresh(winPtr);
}
