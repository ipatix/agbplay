#pragma once

#include <vector>
#include <bitset>

#include "Rom.h"
#include "SampleStructs.h"
#include "Constants.h"

namespace agbplay 
{
    enum class InstrType : int { PCM, PCM_FIXED, SQ1, SQ2, WAVE, NOISE, INVALID };
    class SoundBank
    {
        public:
            SoundBank(Rom& rom, long bankPos);
            ~SoundBank();

            InstrType GetInstrType(uint8_t instrNum, uint8_t midiKey);
            uint8_t GetMidiKey(uint8_t instrNum, uint8_t midiKey);
            uint8_t GetPan(uint8_t instrNum, uint8_t midiKey);
            uint8_t GetSweep(uint8_t instrNum, uint8_t midiKey);
            CGBDef GetCGBDef(uint8_t instrNum, uint8_t midiKey);
            SampleInfo GetSampInfo(uint8_t instrNum, uint8_t midiKey);
            ADSR GetADSR(uint8_t instrNum, uint8_t midiKey);
        private:
            struct Instrument {
                uint8_t type;
                uint8_t midiKey;
                uint8_t hardwareLength; // unsupported
                union { uint8_t pan; uint8_t sweep; } field_3;
                union { uint8_t dutyCycle; agbptr_t wavePtr; agbptr_t samplePtr; agbptr_t subTable; } field_4;
                union { agbptr_t instrMap; ADSR env; } field_8;
            };
            Rom rom;
            long bankPos;
    };

    enum class MODT : int { PITCH = 0, VOL, PAN };
    enum class LEvent : int { NONE = 0, VOL, PAN, BEND, BENDR, MOD, TUNE, XCMD, NOTE, TIE, EOT };
    class Sequence 
    {
        public:
            Sequence(long songHeader, uint8_t trackLimit, Rom& rom);
            ~Sequence();

            struct Track 
            {
                Track(long pos);
                ~Track();
                int16_t GetPitch();
                std::bitset<NUM_NOTES> activeNotes;

                long pos;
                long returnPos;
                long patBegin;
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

            static const std::vector<int16_t> sineLut;
            std::vector<Track> tracks;
            // processing variables
            int32_t bpmStack;
            uint16_t bpm;
            Rom& GetRom();
            long GetSndBnk();
            uint8_t GetReverb();
        private:
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
