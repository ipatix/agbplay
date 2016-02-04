#pragma once

#include <vector>
#include <string>

#include "SongEntry.h"

namespace agbplay
{
    enum class ReverbType 
    {
        NORMAL, GS1, GS2
    };

    class GameConfig
    {
        public:
            GameConfig(std::string gameCode);
            ~GameConfig();

            std::string GetGameCode();
            ReverbType GetRevType();
            void SetRevType(ReverbType revType);
            uint8_t GetPCMVol();
            void SetPCMVol(uint8_t pcmVol);
            uint8_t GetEngineFreq();
            void SetEngineFreq(uint8_t engineFreq);
            uint8_t GetEngineRev();
            void SetEngineRev(uint8_t engineRev);
            uint8_t GetTrackLimit();
            void SetTrackLimit(uint8_t trackLimit);

            std::vector<SongEntry>& GetGameEntries();

        private:
            std::string gameCode;
            std::vector<SongEntry> gameEntries;
            ReverbType revType;
            uint8_t pcmVol;
            uint8_t engineFreq;
            uint8_t engineRev;
            uint8_t trackLimit;
    };
}
