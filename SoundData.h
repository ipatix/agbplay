#pragma once

#include "Rom.h"

#define UNKNOWN_TABLE -1
#define MIN_SONG_NUM 16
#define PROG_UNDEFINED 0xFF

namespace agbplay {
    class SongTable {
        public:
            SongTable(Rom& rrom, long songTable);
            ~SongTable();

            long GetSongTablePos();
            unsigned short GetNumSongs();

        private:
            class Sequence 
            {
                public:
                    Sequence(long songHeader);
                    ~Sequence();
                private:
                    class Track 
                    {
                        public:
                            Track(long pos);
                            ~Track();
                        private:
                            long pos;
                            long retStack[3];
                            long patBegin;
                            uint8_t delay;
                            uint8_t prog;
                            uint8_t vol;
                            uint8_t mod;
                            uint8_t bendr;
                            int8_t pan;
                            int8_t bend;
                            int8_t tune;
                            int8_t keyShift;

                            bool muted;
                    };

                    long songHeader;
                    long soundBank;
                    std::vector<Track> tracks;
                    uint8_t blocks;
                    uint8_t prio;
                    uint8_t reverb;
            };


            long locateSongTable();
            bool validateTableEntry(long pos, bool strongCheck);
            bool validateSong(agbptr_t checkPtr, bool strongCheck);
            unsigned short determineNumSongs();

            Rom& rom;
            long songTable;
            unsigned short numSongs;
    };

    struct SoundData {
        SoundData(Rom& rrom);
        ~SoundData();

        SongTable *sTable;
    };
}
