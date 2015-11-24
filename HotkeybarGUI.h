#pragma once

#include <ncurses.h>
#include "CursesWin.h"

namespace agbplay {
    class HotkeybarGUI : public CursesWin {
        public:
            HotkeybarGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos);
            ~HotkeybarGUI();

            void Resize(uint32_t height, uint32_t width,
                    uint32_t yPos, uint32_t xPos) override;

        private:
            void update() override;
    };
}
