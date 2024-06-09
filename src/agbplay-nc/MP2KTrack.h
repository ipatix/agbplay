#pragma once

#include <bitset>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <vector>

// TODO remove dependency for NUM_NOTES, and possibly remove active notes state?
#include "Constants.h"
#include "Types.h"
#include "LoudnessCalculator.h"

#define TRACK_CALL_STACK_SIZE 3

enum class MODT : int { PITCH = 0, VOL, PAN };

struct MP2KChn;
class ReverbEffect;

struct MP2KTrack
{
    MP2KTrack(size_t pos, uint8_t trackIdx);
    MP2KTrack(const MP2KTrack&) = delete;
    MP2KTrack(MP2KTrack &&) = default;
    MP2KTrack& operator=(const MP2KTrack&) = delete;

    void Init();
    int16_t GetPitch();
    uint16_t GetVol();
    int16_t GetPan();
    void ResetLfoValue();

    std::bitset<NUM_NOTES> activeNotes;
    std::vector<sample> audioBuffer;
    std::unique_ptr<ReverbEffect> reverb;
    LoudnessCalculator loudnessCalculator{5.0f};

    size_t pos;
    size_t returnPos[TRACK_CALL_STACK_SIZE];
    uint8_t patternLevel;
    MODT modt = MODT::PITCH;
    uint8_t lastCmd;
    int16_t pitch;
    uint8_t lastNoteKey;
    uint8_t lastNoteVel;
    uint8_t lastNoteLen;
    uint8_t reptCount;
    uint8_t prog;
    uint8_t vol;
    uint8_t mod;
    uint8_t bendr;
    uint8_t priority;
    uint8_t lfos;
    uint8_t lfodl;
    uint8_t lfodlCount;
    uint8_t lfoPhase;
    int8_t lfoValue;
    uint8_t pseudoEchoVol;
    uint8_t pseudoEchoLen;
    uint16_t delay;
    int8_t pan;
    int8_t bend;
    int8_t tune;
    int8_t keyShift;
    bool muted;
    bool isRunning;
    bool updateVolume;
    bool updatePitch;

    const uint8_t trackIdx;

    MP2KChn *channels = nullptr;
};
