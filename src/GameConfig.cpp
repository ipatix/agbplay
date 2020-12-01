#include "GameConfig.h"
#include "Util.h"

/*
 * public GameConfig
 */

GameConfig::GameConfig(const std::string& gameCode)
{
    gameCodes.push_back(gameCode);
}

GameConfig::GameConfig(const std::vector<std::string>& gameCodes)
{
    this->gameCodes = gameCodes;
}

const std::vector<std::string>& GameConfig::GetGameCodes()
{
    return gameCodes;
}

ReverbType GameConfig::GetRevType()
{
    return revType;
}

void GameConfig::SetRevType(ReverbType revType)
{
    this->revType = revType;
}

ResamplerType GameConfig::GetResTypeFixed()
{
    return resTypeFixed;
}

void GameConfig::SetResTypeFixed(ResamplerType resType)
{
    this->resTypeFixed = resType;
}

ResamplerType GameConfig::GetResType()
{
    return resType;
}

void GameConfig::SetResType(ResamplerType resType)
{
    this->resType = resType;
}

uint8_t GameConfig::GetPCMVol()
{
    return pcmVol;
}

void GameConfig::SetPCMVol(uint8_t pcmVol)
{
    this->pcmVol = std::clamp<uint8_t>(pcmVol, 0, 0xF);
}

uint8_t GameConfig::GetEngineFreq()
{
    return engineFreq;
}

void GameConfig::SetEngineFreq(uint8_t engineFreq)
{
    this->engineFreq = std::clamp<uint8_t>(engineFreq, 1, 12);
}

uint8_t GameConfig::GetEngineRev()
{
    return engineRev;
}

void GameConfig::SetEngineRev(uint8_t engineRev)
{
    this->engineRev = engineRev;
}

uint8_t GameConfig::GetTrackLimit()
{
    return trackLimit;
}

void GameConfig::SetTrackLimit(uint8_t trackLimit)
{
    this->trackLimit = std::clamp<uint8_t>(trackLimit, 0, 16);
}

uint16_t GameConfig::GetRevBufSize()
{
    return revBufSize;
}

void GameConfig::SetRevBufSize(uint16_t revBufSize)
{
    this->revBufSize = revBufSize;
}

std::vector<SongEntry>& GameConfig::GetGameEntries()
{
    return gameEntries;
}
