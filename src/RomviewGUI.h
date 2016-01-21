#pragma once

#include <cstdint>
#include <string>
#include "CursesWin.h"
#include "Rom.h"
#include "SoundData.h"

using namespace std;

namespace agbplay {
    class RomviewGUI : public CursesWin {
        public:
            RomviewGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos, Rom& rrom, SoundData& rsdata);
            ~RomviewGUI();

            void Resize(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos) override;
        private:
            void update() override;
            string gameName;
            string gameCode;
            long songTable;
            unsigned short numSongs;
    };
}
