#pragma once

#include <vector>
#include <bitset>

#include "Types.h"
#include "Constants.h"
#include "Rom.h"

struct PlayerContext;

enum class InstrType { PCM, PCM_FIXED, SQ1, SQ2, WAVE, NOISE, INVALID };
class SoundBank
{
public:
    SoundBank(PlayerContext& ctx);
    SoundBank(const SoundBank&) = delete;
    SoundBank& operator=(const SoundBank&) = delete;

    void Init(size_t bankPos);

    InstrType GetInstrType(uint8_t instrNum, uint8_t midiKey);
    uint8_t GetMidiKey(uint8_t instrNum, uint8_t midiKey);
    int8_t GetPan(uint8_t instrNum, uint8_t midiKey);
    uint8_t GetSweep(uint8_t instrNum, uint8_t midiKey);
    CGBDef GetCGBDef(uint8_t instrNum, uint8_t midiKey);
    SampleInfo GetSampInfo(uint8_t instrNum, uint8_t midiKey);
    ADSR GetADSR(uint8_t instrNum, uint8_t midiKey);
private:
    PlayerContext& ctx;

    size_t instrPos(uint8_t instrNum, uint8_t midiKey);
    size_t bankPos = 0;
};

enum class MODT : int { PITCH = 0, VOL, PAN };

#define TRACK_CALL_STACK_SIZE 3

struct Track
{
    Track(size_t pos);
    Track(const Track&) = delete;
    Track(Track &&) = default;
    Track& operator=(const Track&) = delete;

    int16_t GetPitch();
    uint8_t GetVol();
    int8_t GetPan();
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
    Sequence(PlayerContext& ctx, uint8_t trackLimit);
    Sequence(const Sequence&) = delete;
    Sequence& operator=(const Sequence&) = delete;

    void Init(size_t songHeaderPos);
    void Reset();

    std::vector<Track> tracks;
    std::vector<uint8_t> memaccArea;

    // processing variables
    uint32_t tickCount = 0;
    int32_t bpmStack;
    uint16_t bpm;
    size_t GetSoundBankPos();
    uint8_t GetReverb() const;
    uint8_t GetPriority() const;
    size_t GetSongHeaderPos() const;
private:
    PlayerContext& ctx;

    size_t songHeaderPos;
    uint8_t trackLimit;
}; // end Sequence

class SongTable 
{
public:
    SongTable(Rom& rom, size_t songTablePos = UNKNOWN_TABLE);
    SongTable(const SongTable&) = delete;
    SongTable& operator=(const SongTable&) = delete;

    size_t GetSongTablePos();
    size_t GetPosOfSong(uint16_t uid);
    size_t GetNumSongs();
private:
    Rom& rom;

    size_t locateSongTable();
    bool validateTableEntry(size_t pos);
    bool validateSong(size_t songPos);
    size_t determineNumSongs();

    size_t songTablePos;
    size_t numSongs;
};
