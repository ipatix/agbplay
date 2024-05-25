#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>

#include "Constants.h"
#include "MP2KTrack.h"

class MP2KPlayer
{
public:
    MP2KPlayer(uint8_t trackLimit);
    MP2KPlayer(const MP2KPlayer&) = delete;
    MP2KPlayer& operator=(const MP2KPlayer&) = delete;

    void Init(size_t songHeaderPos);
    void Reset();

    std::vector<MP2KTrack> tracks;

    // processing variables
    uint32_t tickCount = 0;
    int32_t bpmStack = 0;
    uint16_t bpm = 0;
    size_t GetSoundBankPos() const;
    uint8_t GetReverb() const;
    uint8_t GetPriority() const;
    size_t GetSongHeaderPos() const;
private:
    size_t songHeaderPos = 0;
    size_t bankPos = 0;
    const uint8_t trackLimit;
};
