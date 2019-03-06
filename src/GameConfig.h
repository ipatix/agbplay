#pragma once

#include <vector>
#include <string>

#include "SongEntry.h"
#include "Types.h"

namespace agbplay
{
    class GameConfig
    {
        public:
            GameConfig(const std::string& gameCode);
            ~GameConfig();

            const std::string& GetGameCode();
            ReverbType GetRevType();
            void SetRevType(ReverbType revType);
            ResamplerType GetResTypeFixed();
            void SetResTypeFixed(ResamplerType resType);
            ResamplerType GetResType();
            void SetResType(ResamplerType resType);
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
            ResamplerType resTypeFixed;
            ResamplerType resType;
            uint8_t pcmVol;
            uint8_t engineFreq;
            uint8_t engineRev;
            uint8_t trackLimit;
    };
}
