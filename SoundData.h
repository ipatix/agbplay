#pragma once

#include "Rom.h"
#include "DisplayContainer.h"

#define UNKNOWN_TABLE -1
#define MIN_SONG_NUM 32
#define MAX_TRK_CALL 3

namespace agbplay 
{
    enum class InstrType : int { PCM, PCM_FIXED, SQ1, SQ2, WAVE, NOISE, INVALID };
    class SoundBank
    {
        public:
            SoundBank(Rom& rom, long bankPos);
            ~SoundBank();

            InstrType GetInstrType(uint8_t instr, uint8_t midiKey);
            uint8_t GetMidiKey(uint8_t instr, uint8_t midiKey);
            uint8_t GetPan(uint8_t instr, uint8_t midiKey);
            int8_t *GetSamplePtr(uint8_t instr, uint8_t midiKey);
            uint8_t *GetWavePtr(uint8_t instr, uint8_t midiKey);
        private:
            struct Instrument {
                uint8_t instrType;
                uint8_t midiKey;
                uint8_t hardwareLength; // unsupported
                union { uint8_t pan; uint8_t sweep; } field_3;
                union { uint8_t dutyCycle; agbptr_t wave; agbptr_t samplePtr; agbptr_t subTable; } field_4;
                union { agbptr_t instrMap; struct { uint8_t atk, dec, sus, rel; } env; } field_8;
            };
            Rom rom;
            long bankPos;
    };

    enum class MODT : int { PITCH = 0, VOL, PAN };
    enum class LEvent : int { NONE = 0, VOL, PAN, BEND, BENDR, MOD, TUNE, NOTE, TIE, EOT };
    class Sequence 
    {
        public:
            Sequence(long songHeader, uint8_t trackLimit, Rom& rom);
            ~Sequence();

            DisplayContainer& GetUpdatedDisp();
            struct Track 
            {
                Track(long pos);
                ~Track();
                int16_t GetPitch();

                long pos;
                long retStack[MAX_TRK_CALL];
                long patBegin;
                MODT modt;
                LEvent lastEvent;
                uint8_t lastNoteKey;
                uint8_t lastNoteVel;
                uint8_t lastNoteDel;
                uint8_t retStackPos;
                uint8_t reptCount;
                uint8_t prog;
                uint8_t vol;
                uint8_t mod;
                uint8_t bendr;
                uint8_t lfos;
                uint8_t lfodl;
                uint8_t lfodlCount;
                int8_t delay;
                int8_t pan;
                int8_t bend;
                int8_t tune;
                int8_t keyShift;
                bool muted;
                bool isRunning;
            }; // end Track

            std::vector<Track> tracks;
            // processing variables
            int32_t bpmStack;
            uint16_t bpm;
            Rom& getRom();
            long getSndBnk();
        private:
            DisplayContainer dcont;
            Rom rom;
            long songHeader;
            long soundBank;
            uint8_t blocks;
            uint8_t prio;
            uint8_t reverb;
    }; // end Sequence

    class SongTable 
    {
        public:
            SongTable(Rom& rrom, long songTable);
            ~SongTable();

            long GetSongTablePos();
            long GetPosOfSong(uint16_t uid);
            unsigned short GetNumSongs();
        private:
            long locateSongTable();
            bool validateTableEntry(long pos);
            bool validateSong(agbptr_t checkPtr);
            unsigned short determineNumSongs();

            Rom& rom;
            long songTable;
            unsigned short numSongs;
    };

    struct SoundData 
    {
        SoundData(Rom& rrom);
        ~SoundData();

        SongTable *sTable;
    };
}
