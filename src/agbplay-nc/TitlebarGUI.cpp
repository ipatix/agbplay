#include "TitlebarGUI.hpp"

#include "ColorDef.hpp"
#include "Constants.hpp"
#include "Xcept.hpp"

#include <cstring>
#include <string>
#include <vector>

static const std::vector<std::string> bannerText = {
    "           _         _           ",
    " __ _ __ _| |__ _ __| |__ _ _  _ ",
    "/ _` / _` | '_ \\ '_ \\ / _` | || |",
    "\\__,_\\__, |_.__/ .__/_\\__,_|\\_, |",
    "     |___/     |_|          |__/ "
};

TitlebarGUI::TitlebarGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos) :
    CursesWin(height, width, yPos, xPos)
{
    if (width < bannerText[0].size())
        throw Xcept("Terminal too narrow for banner text");
    if (height < bannerText.size())
        throw Xcept("Terminal to flat for banner text");
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
    for (uint32_t i = 0; i < height; i++) {
        // print upper padding
        if (i < upperPadding || i > height - upperPadding) {
            wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::BANNER_TEXT)));
            mvwhline(winPtr, (int)i, 0, ' ', width);
        } else {
            uint32_t leftPadding = (uint32_t)((width - bannerText[i - upperPadding].size()) / 2);
            uint32_t rightPadding = (uint32_t)((width - bannerText[i - upperPadding].size()) / 2
                                               + ((width - bannerText[i - upperPadding].size()) % 2));
            wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::BANNER_TEXT)));
            mvwprintw(
                winPtr, (int)i, 0, "%*s%s%*s", leftPadding, "", bannerText[i - upperPadding].c_str(), rightPadding, ""
            );
        }
    }
    wrefresh(winPtr);
}
