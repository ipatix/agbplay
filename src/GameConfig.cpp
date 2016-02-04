#include "GameConfig.h"
#include "Util.h"

using namespace std;
using namespace agbplay;

/*
 * public GameConfig
 */

GameConfig::GameConfig(string gameCode)
{
    this->gameCode = gameCode;
    revType = ReverbType::NORMAL;
    pcmVol = 0xF;
    engineFreq = 0x4;
    engineRev = 0x0;
}

GameConfig::~GameConfig()
{
}

string GameConfig::GetGameCode()
{
    return gameCode;
}

ReverbType GameConfig::GetRevType()
{
    return revType;
}

void GameConfig::SetRevType(ReverbType revType)
{
    this->revType = revType;
}

uint8_t GameConfig::GetPCMVol()
{
    return pcmVol;
}

void GameConfig::SetPCMVol(uint8_t pcmVol)
{
    this->pcmVol = minmax<uint8_t>(0, pcmVol, 0xF);
}

uint8_t GameConfig::GetEngineFreq()
{
    return engineFreq;
}

void GameConfig::SetEngineFreq(uint8_t engineFreq)
{
    this->engineFreq = minmax<uint8_t>(1, engineFreq, 12);
}

uint8_t GameConfig::GetEngineRev()
{
    return engineRev;
}

void GameConfig::SetEngineRev(uint8_t engineRev)
{
    this->engineRev = engineRev;
}

vector<SongEntry>& GameConfig::GetGameEntries()
{
    return gameEntries;
}
