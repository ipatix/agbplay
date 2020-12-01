#pragma once

#include <bitset>
#include <vector>

#include "Constants.h"

struct DisplayData 
{
    DisplayData();
    ~DisplayData();

    uint32_t trackPtr;
    bool isCalling;
    bool isMuted;
    uint8_t vol;   // range 0 to 127
    uint8_t mod;   // range 0 to 127
    uint8_t prog;  // range 0 to 127
    int8_t pan;    // range -64 to 63
    int16_t pitch; // range -32768 to 32767
    uint8_t envL;  // range 0 to 255
    uint8_t envR;  // range 0 to 255
    int8_t delay; // range 0 to 96
    std::bitset<NUM_NOTES> activeNotes;
};

struct DisplayContainer 
{
    DisplayContainer();
    DisplayContainer(uint8_t nTracks);
    ~DisplayContainer();

    std::vector<DisplayData> data;
};
