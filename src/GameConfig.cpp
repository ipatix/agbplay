#include "GameConfig.h"
#include "Util.h"

#include <algorithm>

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

const std::vector<std::string>& GameConfig::GetGameCodes() const
{
    return gameCodes;
}

ReverbType GameConfig::GetRevType() const
{
    return revType;
}

void GameConfig::SetRevType(ReverbType revType)
{
    this->revType = revType;
}

ResamplerType GameConfig::GetResTypeFixed() const
{
    return resTypeFixed;
}

void GameConfig::SetResTypeFixed(ResamplerType resType)
{
    this->resTypeFixed = resType;
}

ResamplerType GameConfig::GetResType() const
{
    return resType;
}

void GameConfig::SetResType(ResamplerType resType)
{
    this->resType = resType;
}

uint8_t GameConfig::GetPCMVol() const
{
    return pcmVol;
}

void GameConfig::SetPCMVol(uint8_t pcmVol)
{
    this->pcmVol = std::clamp<uint8_t>(pcmVol, 0, 0xF);
}

uint8_t GameConfig::GetEngineFreq() const
{
    return engineFreq;
}

void GameConfig::SetEngineFreq(uint8_t engineFreq)
{
    this->engineFreq = std::clamp<uint8_t>(engineFreq, 1, 12);
}

uint8_t GameConfig::GetEngineRev() const
{
    return engineRev;
}

void GameConfig::SetEngineRev(uint8_t engineRev)
{
    this->engineRev = engineRev;
}

uint8_t GameConfig::GetTrackLimit() const
{
    return trackLimit;
}

void GameConfig::SetTrackLimit(uint8_t trackLimit)
{
    this->trackLimit = std::clamp<uint8_t>(trackLimit, 0, 16);
}

uint16_t GameConfig::GetRevBufSize() const
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

bool GameConfig::GetAccurateCh3Volume() const
{
    return accurateCh3Volume;
}

void GameConfig::SetAccurateCh3Volume(bool enabled)
{
    this->accurateCh3Volume = enabled;
}

bool GameConfig::GetAccurateCh3Quantization() const
{
    return accurateCh3Quantization;
}

void GameConfig::SetAccurateCh3Quantization(bool enabled)
{
    this->accurateCh3Quantization = enabled;
}

bool GameConfig::GetSimulateCGBSustainBug() const
{
    return simulateCGBSustainBug;
}

void GameConfig::SetSimulateCGBSustainBug(bool enabled)
{
    this->simulateCGBSustainBug = enabled;
}
