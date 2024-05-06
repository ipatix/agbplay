#pragma once

#include <bitset>
#include <vector>
#include <array>

#include "Constants.h"

struct PlaybackSongState 
{
    PlaybackSongState() = default;
    PlaybackSongState(const PlaybackSongState&) = delete;
    //PlaybackSongState& operator=(const PlaybackSongState&) = delete;

    struct TrackState
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
        uint8_t delay = 0;              // range 0 to 96
        std::bitset<NUM_NOTES> activeNotes;
    };

    std::array<TrackState, MAX_TRACKS> tracks;
    size_t tracks_used = 0;

    float masterVolLeft = 0.0f, masterVolRight = 0.0f;
    size_t activeChannels = 0;
    size_t maxChannels = 0;
};
