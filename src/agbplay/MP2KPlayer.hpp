#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>

#include "Constants.hpp"
#include "MP2KTrack.hpp"

class Rom;

struct MP2KPlayer
{
    MP2KPlayer(const PlayerInfo &playerInfo, uint8_t playerIdx);
    MP2KPlayer(const MP2KPlayer&) = delete;
    MP2KPlayer(MP2KPlayer &&) = default;
    MP2KPlayer& operator=(const MP2KPlayer&) = delete;

    void Init(const Rom &rom, size_t songHeaderPos);

    std::vector<MP2KTrack> tracks;

    /* playback state */
    bool playing = false;
    bool finished = true;

    /* player timing state */
    size_t interframeCount = 0;
    size_t frameCount = 0;
    size_t tickCount = 0;
    int32_t bpmStack = 0;
    uint16_t bpm = 0;

    /* variables initialized by song start */
    size_t songHeaderPos = 0;
    size_t bankPos = 0;
    uint8_t tracksUsed = 0;
    uint8_t reverb = 0;
    uint8_t priority = 0;

    /* player constants */
    const uint8_t trackLimit;
    const uint8_t playerIdx;
    const bool usePriority;
};
