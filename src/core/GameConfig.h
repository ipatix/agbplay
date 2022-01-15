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

    const std::vector<std::string>& GetGameCodes() const;
    ReverbType GetRevType() const;
    void SetRevType(ReverbType revType);
    ResamplerType GetResTypeFixed() const;
    void SetResTypeFixed(ResamplerType resType);
    ResamplerType GetResType() const;
    void SetResType(ResamplerType resType);
    uint8_t GetPCMVol() const;
    void SetPCMVol(uint8_t pcmVol);
    uint8_t GetEngineFreq() const;
    void SetEngineFreq(uint8_t engineFreq);
    uint8_t GetEngineRev() const;
    void SetEngineRev(uint8_t engineRev);
    uint8_t GetTrackLimit() const;
    void SetTrackLimit(uint8_t trackLimit);
    uint16_t GetRevBufSize() const;
    void SetRevBufSize(uint16_t revBufSize);
    bool GetAccurateCh3Volume() const;
    void SetAccurateCh3Volume(bool enabled);
    bool GetAccurateCh3Quantization() const;
    void SetAccurateCh3Quantization(bool enabled);

    std::vector<SongEntry>& GetGameEntries();

private:
    std::vector<std::string> gameCodes;
    std::vector<SongEntry> gameEntries;
    ReverbType revType = ReverbType::NORMAL;
    ResamplerType resTypeFixed = ResamplerType::LINEAR;
    ResamplerType resType = ResamplerType::LINEAR;
    uint8_t pcmVol = 0xF;
    uint8_t engineFreq = 0x4;
    uint8_t engineRev = 0x0;
    uint8_t trackLimit = 16;
    uint16_t revBufSize = 1584;
    bool accurateCh3Volume = false;
    bool accurateCh3Quantization = false;
};
