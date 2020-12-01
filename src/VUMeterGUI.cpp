#include <string>
#include <cstring>
#include <cmath>

#include "VUMeterGUI.h"
#include "ColorDef.h"
#include "Util.h"
#include "Xcept.h"
#include "Debug.h"

using namespace std;

/*
 * public VUMeterGUI
 */

VUMeterGUI::VUMeterGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos)
    : CursesWin(height, width, yPos, xPos)
{
    if (width < 10)
        throw Xcept("Can't create too narrow VU meters");
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
        throw Xcept("Can't create too narrow VU meters");
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
    char line[(meterWidth + 2) * strlen("\u250f") + 1];
    size_t currentLinePos = 0;
    float levelFactor = clip(0.0f, 0.8f * float(meterWidth), float(meterWidth));

    // TOP BORDER
    CStrAppend(line, &currentLinePos, "\u250f");
    for (int i = 0; i < meterWidth; i++) {
        CStrAppend(line, &currentLinePos, "\u2501");
    }
    CStrAppend(line, &currentLinePos, "\u2513");
    line[currentLinePos] = '\0';
    wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::WINDOW_FRAME)));
    mvwprintw(winPtr, 0, 0, "%s", line);

    // FIRST BAR
    float leftLevel = vuLevelLeft * levelFactor;
    int bLeftLevel = int(leftLevel);
    mvwprintw(winPtr, 1, 0, "\u2503");

    currentLinePos = 0;
    for (int i = 0; i < meterYel; i++) {
        if (i < bLeftLevel) {
            CStrAppend(line, &currentLinePos, "\u2588");
        } else {
            CStrAppend(line, &currentLinePos, "\u2591");
        }
    }
    line[currentLinePos] = '\0';
    wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::VU_LOW)));
    wprintw(winPtr, "%s", line);

    currentLinePos = 0;
    for (int i = meterYel; i < meterRed; i++) {
        if (i < bLeftLevel) {
            CStrAppend(line, &currentLinePos, "\u2588");
        } else {
            CStrAppend(line, &currentLinePos, "\u2591");
        }
    }
    line[currentLinePos] = '\0';
    wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::VU_MID)));
    wprintw(winPtr, "%s", line);

    currentLinePos = 0;
    for (int i = meterRed; i < meterWidth; i++) {
        if (i < bLeftLevel) {
            CStrAppend(line, &currentLinePos, "\u2588");
        } else {
            CStrAppend(line, &currentLinePos, "\u2591");
        }
    }
    line[currentLinePos] = '\0';
    wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::VU_HIGH)));
    wprintw(winPtr, "%s", line);

    wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::WINDOW_FRAME)));
    wprintw(winPtr, "\u2503");

    // MIDDLE BORDER
    currentLinePos = 0;
    CStrAppend(line, &currentLinePos, "\u2523");
    for (int i = 0; i < meterWidth; i++) {
        CStrAppend(line, &currentLinePos, "\u2501");
    }
    CStrAppend(line, &currentLinePos, "\u252b");
    line[currentLinePos] = '\0';
    mvwprintw(winPtr, 2, 0, "%s", line);

    // SECOND BAR
    float rightLevel = vuLevelRight * levelFactor;
    int bRightLevel = int(rightLevel);
    wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::WINDOW_FRAME)));
    mvwprintw(winPtr, 3, 0, "\u2503");

    currentLinePos = 0;
    for (int i = 0; i < meterYel; i++) {
        if (i < bRightLevel) {
            CStrAppend(line, &currentLinePos, "\u2588");
        } else {
            CStrAppend(line, &currentLinePos, "\u2591");
        }
    }
    line[currentLinePos] = '\0';
    wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::VU_LOW)));
    wprintw(winPtr, "%s", line);

    currentLinePos = 0;
    for (int i = meterYel; i < meterRed; i++) {
        if (i < bRightLevel) {
            CStrAppend(line, &currentLinePos, "\u2588");
        } else {
            CStrAppend(line, &currentLinePos, "\u2591");
        }
    }
    line[currentLinePos] = '\0';
    wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::VU_MID)));
    wprintw(winPtr, "%s", line);

    currentLinePos = 0;
    for (int i = meterRed; i < meterWidth; i++) {
        if (i < bRightLevel) {
            CStrAppend(line, &currentLinePos, "\u2588");
        } else {
            CStrAppend(line, &currentLinePos, "\u2591");
        }
    }
    line[currentLinePos] = '\0';
    wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::VU_HIGH)));
    wprintw(winPtr, "%s", line);

    wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::WINDOW_FRAME)));
    wprintw(winPtr, "\u2503");

    // BOTTOM BORDER
    currentLinePos = 0;
    CStrAppend(line, &currentLinePos, "\u2523");
    for (int i = 0; i < meterWidth; i++) {
        CStrAppend(line, &currentLinePos, "\u2501");
    }
    CStrAppend(line, &currentLinePos, "\u252b");
    line[currentLinePos] = '\0';
    mvwprintw(winPtr, 4, 0, "%s", line);
    wrefresh(winPtr);
}
