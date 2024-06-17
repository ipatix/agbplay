#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>

#include "Constants.h"
#include "MP2KTrack.h"

class MP2KPlayer
{
public:
    MP2KPlayer(const PlayerInfo &playerInfo, uint8_t playerIdx);
    MP2KPlayer(const MP2KPlayer&) = delete;
    MP2KPlayer(MP2KPlayer &&) = default;
    MP2KPlayer& operator=(const MP2KPlayer&) = delete;

    void Init(size_t songHeaderPos);
    void Reset();

    std::vector<MP2KTrack> tracks;

    // processing variables
    size_t tickCount = 0;
    int32_t bpmStack = 0;
    uint16_t bpm = 0;
    const uint8_t playerIdx;
    bool enabled = false;
    uint8_t tracksUsed = 0;

    size_t GetSoundBankPos() const;
    uint8_t GetReverb() const;
    uint8_t GetPriority() const;
    size_t GetSongHeaderPos() const;
private:
    size_t songHeaderPos = 0;
    size_t bankPos = 0;
    const uint8_t trackLimit;
    const bool usePriority; // TODO actually implement the usage of this flag
};
