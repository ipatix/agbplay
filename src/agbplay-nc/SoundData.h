#pragma once

#include <vector>
#include <bitset>

#include "Types.h"
#include "Constants.h"

const uint8_t BANKDATA_TYPE_SPLIT = 0x40;
const uint8_t BANKDATA_TYPE_RHYTHM = 0x80;

const uint8_t BANKDATA_TYPE_CGB = 0x07;
const uint8_t BANKDATA_TYPE_FIX = 0x08;

const uint8_t BANKDATA_TYPE_PCM = 0x00;
const uint8_t BANKDATA_TYPE_SQ1 = 0x01;
const uint8_t BANKDATA_TYPE_SQ2 = 0x02;
const uint8_t BANKDATA_TYPE_WAVE = 0x03;
const uint8_t BANKDATA_TYPE_NOISE = 0x04;

enum class MODT : int { PITCH = 0, VOL, PAN };

#define TRACK_CALL_STACK_SIZE 3

struct Track
{
    Track(size_t pos);
    Track(const Track&) = delete;
    Track(Track &&) = default;
    Track& operator=(const Track&) = delete;

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
}; // end Track

class Sequence
{
public:
    Sequence(uint8_t trackLimit);
    Sequence(const Sequence&) = delete;
    Sequence& operator=(const Sequence&) = delete;

    void Init(size_t songHeaderPos);
    void Reset();

    std::vector<Track> tracks;
    std::vector<uint8_t> memaccArea;

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
    uint8_t trackLimit;
}; // end Sequence

class SongTable
{
public:
    static std::vector<SongTable> ScanForTables();
    SongTable(size_t songTablePos);
    SongTable(const SongTable&) = default;
    SongTable& operator=(const SongTable&) = default;

    size_t GetSongTablePos();
    size_t GetPosOfSong(uint16_t uid);
    size_t GetNumSongs();
private:
    static bool validateTableEntry(size_t pos);
    static bool validateSong(size_t songPos);
    size_t determineNumSongs() const;

    size_t songTablePos;
    size_t numSongs;
};
