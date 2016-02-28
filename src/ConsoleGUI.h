#pragma once

#define CONSOLE_BORDER_WIDTH 2

#include <vector>
#include <string>

#include "CursesWin.h"

namespace agbplay {
    class ConsoleGUI : public CursesWin {
        public:
            ConsoleGUI(uint32_t height, uint32_t width, 
                    uint32_t yPos, uint32_t xPos);
            ~ConsoleGUI();
            void Resize(uint32_t height, uint32_t width,
                    uint32_t yPos, uint32_t xPos) override;
            void WriteLn(std::string str);
        private:
            void update() override;
            void writeToBuffer(std::string str);

            uint32_t textWidth, textHeight;
            std::vector<std::string> textBuffer;
    };
}
