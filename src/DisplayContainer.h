#pragma once

#include <bitset>
#include <vector>

#include "Constants.h"

struct DisplayData 
{
    uint32_t trackPtr = 0;
    bool isCalling = false;
    bool isMuted = false;
    uint8_t vol = 100;              // range 0 to 127
    uint8_t mod = 0;                // range 0 to 127
    uint8_t prog = PROG_UNDEFINED;  // range 0 to 127
    int8_t pan = 0;                 // range -64 to 63
    int16_t pitch = 0;              // range -32768 to 32767
    uint8_t envL = 0;               // range 0 to 255
    uint8_t envR = 0;               // range 0 to 255
    int8_t delay = 0;               // range 0 to 96
    std::bitset<NUM_NOTES> activeNotes;
};

struct DisplayContainer 
{
    DisplayContainer() = default;
    DisplayContainer(const DisplayContainer&) = delete;
    DisplayContainer& operator=(const DisplayContainer&) = delete;

    std::vector<DisplayData> data;
};
