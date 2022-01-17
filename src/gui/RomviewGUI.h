#pragma once

#include <cstdint>
#include <string>
#include "CursesWin.h"
#include "Rom.h"
#include "SoundData.h"

class RomviewGUI : public CursesWin {
public:
    RomviewGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos, SongTable& songTable);
    ~RomviewGUI() override;

    void Resize(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos) override;
private:
    void update() override;
    std::string gameName;
    std::string gameCode;
    size_t songTablePos;
    size_t numSongs;
};
