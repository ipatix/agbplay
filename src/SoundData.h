#pragma once

#include <vector>
#include <bitset>

#include "Types.h"
#include "Constants.h"

enum class InstrType { PCM, PCM_FIXED, SQ1, SQ2, WAVE, NOISE, INVALID };
class SoundBank
{
public:
    SoundBank() = default;
    SoundBank(const SoundBank&) = delete;
    SoundBank& operator=(const SoundBank&) = delete;

    void Init(size_t bankPos);

    InstrType GetInstrType(uint8_t instrNum, uint8_t midiKey);
    uint8_t GetMidiKey(uint8_t instrNum, uint8_t midiKey);
    uint8_t GetPan(uint8_t instrNum, uint8_t midiKey);
    uint8_t GetSweep(uint8_t instrNum, uint8_t midiKey);
    CGBDef GetCGBDef(uint8_t instrNum, uint8_t midiKey);
    SampleInfo GetSampInfo(uint8_t instrNum, uint8_t midiKey);
    ADSR GetADSR(uint8_t instrNum, uint8_t midiKey);
private:
    size_t instrPos(uint8_t instrNum, uint8_t midiKey);
    size_t bankPos = 0;
};

enum class MODT : int { PITCH = 0, VOL, PAN };
enum class LEvent : int { NONE = 0, VOICE, VOL, PAN, BEND, BENDR, MOD, TUNE, XCMD, NOTE, TIE, EOT };

struct Track
{
    Track(size_t pos);
    Track(const Track&) = delete;
    Track(Track &&) = default;
    Track& operator=(const Track&) = delete;

    int16_t GetPitch();
    uint8_t GetVol();
    int8_t GetPan();
    std::bitset<NUM_NOTES> activeNotes;

    size_t pos;
    size_t returnPos = 0;
    size_t patBegin = 0;
    MODT modt = MODT::PITCH;
    LEvent lastEvent = LEvent::NONE;
    int16_t pitch = 0;
    uint8_t lastNoteKey = 60, lastNoteVel = 127;
    int8_t lastNoteLen = 96;
    uint8_t reptCount = 0;
    uint8_t prog = PROG_UNDEFINED, vol = 100, mod = 0, bendr = 2, prio = 0;
    uint8_t lfos = 22, lfodl = 0, lfodlCount = 0, lfoPhase = 0;
    uint8_t echoVol = 0, echoLen = 0;
    int8_t delay = 0;
    int8_t pan = 0, bend = 0, tune = 0;
    int8_t keyShift = 0;
    bool muted = false;
    bool isRunning = true;
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
    // processing variables
    int32_t bpmStack;
    uint16_t bpm;
    size_t GetSoundBankPos();
    uint8_t GetReverb() const;
    size_t GetSongHeaderPos() const;
private:
    size_t songHeaderPos;
    uint8_t trackLimit;
}; // end Sequence

class SongTable 
{
public:
    SongTable(size_t songTablePos = UNKNOWN_TABLE);
    SongTable(const SongTable&) = delete;
    SongTable& operator=(const SongTable&) = delete;

    size_t GetSongTablePos();
    size_t GetPosOfSong(uint16_t uid);
    size_t GetNumSongs();
private:
    size_t locateSongTable();
    bool validateTableEntry(size_t pos);
    bool validateSong(size_t songPos);
    size_t determineNumSongs();

    size_t songTablePos;
    size_t numSongs;
};
