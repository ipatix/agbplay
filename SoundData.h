#pragma once

#include "Rom.h"
#include "DisplayContainer.h"

#define UNKNOWN_TABLE -1
#define MIN_SONG_NUM 32
#define MAX_TRK_CALL 3

namespace agbplay 
{
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
            };
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
    };

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
