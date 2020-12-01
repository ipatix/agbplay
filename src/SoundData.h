#pragma once

#include <vector>
#include <bitset>

#include "Types.h"
#include "Constants.h"

enum class InstrType { PCM, PCM_FIXED, SQ1, SQ2, WAVE, NOISE, INVALID };
class SoundBank
{
public:
    SoundBank(size_t bankPos);
    SoundBank(const SoundBank&) = delete;
    SoundBank& operator=(const SoundBank&) = delete;

    InstrType GetInstrType(uint8_t instrNum, uint8_t midiKey);
    uint8_t GetMidiKey(uint8_t instrNum, uint8_t midiKey);
    uint8_t GetPan(uint8_t instrNum, uint8_t midiKey);
    uint8_t GetSweep(uint8_t instrNum, uint8_t midiKey);
    CGBDef GetCGBDef(uint8_t instrNum, uint8_t midiKey);
    SampleInfo GetSampInfo(uint8_t instrNum, uint8_t midiKey);
    ADSR GetADSR(uint8_t instrNum, uint8_t midiKey);
private:
    size_t instrPos(uint8_t instrNum, uint8_t midiKey);
    size_t bankPos;
};

enum class MODT : int { PITCH = 0, VOL, PAN };
enum class LEvent : int { NONE = 0, VOICE, VOL, PAN, BEND, BENDR, MOD, TUNE, XCMD, NOTE, TIE, EOT };
class Sequence 
{
public:
    Sequence(size_t songHeaderPos, uint8_t trackLimit);
    Sequence(const Sequence&) = delete;
    Sequence& operator=(const Sequence&) = delete;

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
        size_t returnPos;
        size_t patBegin;
        MODT modt;
        LEvent lastEvent;
        int16_t pitch;
        uint8_t lastNoteKey, lastNoteVel;
        int8_t lastNoteLen;
        uint8_t reptCount;
        uint8_t prog, vol, mod, bendr, prio;
        uint8_t lfos, lfodl, lfodlCount, lfoPhase;
        uint8_t echoVol, echoLen;
        int8_t delay;
        int8_t pan, bend, tune;
        int8_t keyShift;
        bool muted;
        bool isRunning;
    }; // end Track

    static const std::vector<int16_t> triLut;
    std::vector<Track> tracks;
    // processing variables
    int32_t bpmStack;
    uint16_t bpm;
    size_t GetSoundBankPos();
    uint8_t GetReverb();
private:
    size_t songHeaderPos;
}; // end Sequence

class SongTable 
{
public:
    SongTable(size_t songTablePos);
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

struct SoundData 
{
    SoundData();
    SoundData(const SoundData&) = delete;
    SoundData& operator=(const SoundData&) = delete;

    SongTable sTable;
};
