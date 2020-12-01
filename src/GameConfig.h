#pragma once

#include <vector>
#include <string>

#include "SongEntry.h"
#include "Types.h"

class GameConfig
{
public:
    GameConfig(const std::string& gameCode);
    GameConfig(const std::vector<std::string>& gameCodes);
    GameConfig(const GameConfig&) = delete;
    GameConfig(GameConfig&&) = default;
    GameConfig& operator=(const GameConfig&) = delete;
    ~GameConfig();

    const std::vector<std::string>& GetGameCodes();
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
    uint16_t GetRevBufSize();
    void SetRevBufSize(uint16_t revBufSize);

    std::vector<SongEntry>& GetGameEntries();

private:
    std::vector<std::string> gameCodes;
    std::vector<SongEntry> gameEntries;
    ReverbType revType;
    ResamplerType resTypeFixed;
    ResamplerType resType;
    uint8_t pcmVol;
    uint8_t engineFreq;
    uint8_t engineRev;
    uint8_t trackLimit;
    uint16_t revBufSize;
};
