#include <string>
#include <cmath>

#include "VUMeterGUI.h"
#include "ColorDef.h"
#include "Util.h"
#include "MyException.h"
#include "Debug.h"

using namespace agbplay;
using namespace std;

/*
 * public VUMeterGUI
 */

VUMeterGUI::VUMeterGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos)
    : CursesWin(height, width, yPos, xPos)
{
    if (width < 10)
        throw MyException("Can't create too narrow VU meters");
    meterWidth = int(width - 2);
    meterRed = meterWidth * 7 / 8;
    meterYel = meterWidth * 6 / 8;
    vuLevelLeft = 0.0f;
    vuLevelRight = 0.0f;
    update();
}

VUMeterGUI::~VUMeterGUI()
{
}

void VUMeterGUI::Resize(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos)
{
    CursesWin::Resize(height, width, yPos, xPos);
    if (width < 10)
        throw MyException("Can't create too narrow VU meters");
    meterWidth = int(width - 2);
    meterRed = meterWidth * 7 / 8;
    meterYel = meterWidth * 6 / 8;
    update();
}

void VUMeterGUI::SetVol(float left, float right)
{
    //vuLevelLeft = left <= 0.0f ? 0.0f : max(log2f(left) * (1.0f / 6.0f) + 1.0f, 0.0f);
    //vuLevelRight = right <= 0.0f ? 0.0f : max(log2f(left) * (1.0f / 6.0f) + 1.0f, 0.0f);
    vuLevelLeft = left;
    vuLevelRight = right;
    update();
}

void VUMeterGUI::update()
{
    string line;
    float levelFactor = minmax(0.0f, 0.8f * float(meterWidth), float(meterWidth));
    // draw top border
    wattrset(winPtr, COLOR_PAIR(Color::WINDOW_FRAME));
    line = "\u250f";
    for (int i = 0; i < meterWidth; i++)
        line += "\u2501";
    line += "\u2513";
    mvwprintw(winPtr, 0, 0, "%s", line.c_str());

    // first bar
    line = "\u2503";
    float leftLevel = vuLevelLeft * levelFactor;
    int bLeftLevel = int(leftLevel);
    mvwprintw(winPtr, 1, 0, "%s", line.c_str());

    line.clear();
    wattrset(winPtr, COLOR_PAIR(Color::VU_LOW));
    for (int i = 0; i < meterYel; i++) {
        if (i < bLeftLevel) {
            line += "\u2588";
        } else {
            line += "\u2591";
        }
    }
    wprintw(winPtr, "%s", line.c_str());

    line.clear();
    wattrset(winPtr, COLOR_PAIR(Color::VU_MID));
    for (int i = meterYel; i < meterRed; i++) {
        if (i < bLeftLevel) {
            line += "\u2588";
        } else {
            line += "\u2591";
        }
    }
    wprintw(winPtr, "%s", line.c_str());

    line.clear();
    wattrset(winPtr, COLOR_PAIR(Color::VU_HIGH));
    for (int i = meterRed; i < meterWidth; i++) {
        if (i < bLeftLevel) {
            line += "\u2588";
        } else {
            line += "\u2591";
        }
    }
    wprintw(winPtr, "%s", line.c_str());

    wattrset(winPtr, COLOR_PAIR(Color::WINDOW_FRAME));
    line = "\u2503";
    wprintw(winPtr, "%s", line.c_str());

    // middle border
    line = "\u2523";
    for (int i = 0; i < meterWidth; i++)
        line += "\u2501";
    line += "\u252b";
    mvwprintw(winPtr, 2, 0, "%s", line.c_str());

    // second bar
    line = "\u2503";
    float rightLevel = vuLevelRight * levelFactor;
    int bRightLevel = int(rightLevel);
    wattrset(winPtr, COLOR_PAIR(Color::WINDOW_FRAME));
    mvwprintw(winPtr, 3, 0, "%s", line.c_str());

    line.clear();
    wattrset(winPtr, COLOR_PAIR(Color::VU_LOW));
    for (int i = 0; i < meterYel; i++) {
        if (i < bRightLevel) {
            line += "\u2588";
        } else {
            line += "\u2591";
        }
    }
    wprintw(winPtr, "%s", line.c_str());

    line.clear();
    wattrset(winPtr, COLOR_PAIR(Color::VU_MID));
    for (int i = meterYel; i < meterRed; i++) {
        if (i < bRightLevel) {
            line += "\u2588";
        } else {
            line += "\u2591";
        }
    }
    wprintw(winPtr, "%s", line.c_str());

    line.clear();
    wattrset(winPtr, COLOR_PAIR(Color::VU_HIGH));
    for (int i = meterRed; i < meterWidth; i++) {
        if (i < bRightLevel) {
            line += "\u2588";
        } else {
            line += "\u2591";
        }
    }
    wprintw(winPtr, "%s", line.c_str());

    wattrset(winPtr, COLOR_PAIR(Color::WINDOW_FRAME));
    line = "\u2503";
    wprintw(winPtr, "%s", line.c_str());

    // bottom border
    line = "\u2523";
    for (int i = 0; i < meterWidth; i++)
        line += "\u2501";
    line += "\u252b";
    mvwprintw(winPtr, 4, 0, "%s", line.c_str());
    wrefresh(winPtr);
}
