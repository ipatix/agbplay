#pragma once

#define CONSOLE_BORDER_WIDTH 2

#include <vector>

#include "CursesWin.h"

using namespace std;

namespace agbplay {
    class ConsoleGUI : public CursesWin {
        public:
            ConsoleGUI(uint32_t height, uint32_t width, 
                    uint32_t yPos, uint32_t xPos);
            ~ConsoleGUI();
            void Resize(uint32_t height, uint32_t width,
                    uint32_t yPos, uint32_t xPos) override;
            void WriteLn(string str);
            int ConGetCH();
        private:
            void update() override;
            void writeToBuffer(string str);

            uint32_t textWidth, textHeight;
            vector<string> textBuffer;
    };
}
