#pragma once

#include "CursesWin.h"

namespace agbplay
{
    class VUMeterGUI : public CursesWin
    {
        public:
            VUMeterGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos);
            ~VUMeterGUI();
            void Resize(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos);
        private:
            update();
    };
}
