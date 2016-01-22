#include "VUMeterGUI.h"

using namespace agbplay;

/*
 * public VUMeterGUI
 */

VUMeterGUI::VUMeterGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos)
    : CursesWin(height, width, yPos, xPos)
{
}

VUMeterGUI::~VUMeterGUI()
{
}

void VUMeterGUI::Resize(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos)
{
    CursesWin::Resize(height, width, yPos, xPos);
}

void VUMeterGUI::update()
{
}
