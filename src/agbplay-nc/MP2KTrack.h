#pragma once

#include <bitset>
#include <cstdint>
#include <cstddef>

// TODO remove dependency for NUM_NOTES, and possibly remove active notes state?
#include "Constants.h"


enum class MODT : int { PITCH = 0, VOL, PAN };
#define TRACK_CALL_STACK_SIZE 3

struct MP2KTrack
{
    MP2KTrack(size_t pos);
    MP2KTrack(const MP2KTrack&) = delete;
    MP2KTrack(MP2KTrack &&) = default;
    MP2KTrack& operator=(const MP2KTrack&) = delete;

    int16_t GetPitch();
    uint16_t GetVol();
    int16_t GetPan();
    void ResetLfoValue();

    std::bitset<NUM_NOTES> activeNotes;

    size_t pos;
    size_t returnPos[TRACK_CALL_STACK_SIZE];
    uint8_t patternLevel = 0;
    MODT modt = MODT::PITCH;
    uint8_t lastCmd = 0;
    int16_t pitch = 0;
    uint8_t lastNoteKey = 60, lastNoteVel = 127;
    uint8_t lastNoteLen = 96;
    uint8_t reptCount = 0;
    uint8_t prog = PROG_UNDEFINED, vol = 100, mod = 0, bendr = 2, priority = 0;
    uint8_t lfos = 22, lfodl = 0, lfodlCount = 0, lfoPhase = 0;
    int8_t lfoValue = 0;
    uint8_t pseudoEchoVol = 0, pseudoEchoLen = 0;
    uint16_t delay = 0;
    int8_t pan = 0, bend = 0, tune = 0;
    int8_t keyShift = 0;
    bool muted = false;
    bool isRunning = true;
    bool updateVolume = false;
    bool updatePitch = false;
};
