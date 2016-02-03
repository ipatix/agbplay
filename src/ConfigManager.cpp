#include "ConfigManager.h"
#include "Util.h"

using namespace std;
using namespace agbplay;

ConfigManager::ConfigManager(std::string configPath)
{
    // parse things from config file
    ifstream configFile(configPath);
    if (!configFile.is_open()) {
        throw MyException(FormatString("Error while opening config file: %s", strerror(errno)));
    }
    string line;
    GameConfig *currentGame = nullptr;
    regex gameExpr("^\\s*\\[([0-9A-Z]{4})\\]\\s*$");
    regex songExpr("^\\s*(\\d+)\\s*=\\s*(.*)$");
    regex cfgVolExpr("^\\s*ENG_VOL\\s*=\\s*(\\d+)\\s*$");
    regex cfgFreqExpr("^\\s*ENG_FREQ\\s*=\\s*(\\d+)\\s*$");
    regex cfgRevExpr("^\\s*ENG_REV\\s*=\\s*(\\d+)\\s*$");
    regex cfgRevTypeExpr("^\\s*ENG_REV_TYPE\\s*=\\s*(.*)\\s*$");

    while (getline(configFile, line)) {
        if (configFile.bad()) {
            throw MyException(FormatString("Error while reading config file: %s", strerror(errno)));
        }
        smatch sm;
        if (regex_match(line, sm, songExpr) && sm.size() == 3 && currentGame != nullptr) {
            currentGame->GetGameEntries().push_back(SongEntry(sm[2], (uint16_t(stoi(sm[1])))));
        }
        else if (regex_match(line, sm, gameExpr) && sm.size() == 2) {
            currentGame = &GetConfig(sm[1]);
        }
        else if (regex_match(line, sm, cfgVolExpr) && sm.size() == 2 && currentGame != nullptr) {
            currentGame->SetPCMVol(uint8_t(minmax<int>(0, stoi(sm[1]), 15)));
        }
        else if (regex_match(line, sm, cfgFreqExpr) && sm.size() == 2 && currentGame != nullptr) {
            currentGame->SetEngineFreq(uint8_t(minmax<int>(0, stoi(sm[1]), 15)));
        }
        else if (regex_match(line, sm, cfgRevExpr) && sm.size() == 2 && currentGame != nullptr) {
            currentGame->SetEngineRev(uint8_t(minmax<int>(0, stoi(sm[1]), 255)));
        }
        else if (regex_match(line, sm, cfgRevTypeExpr) && sm.size() == 2 && currentGame != nullptr) {
            string& res = sm[1];
            if (res == "NORMAL") {
                currentGame->SetRevType(ReverbType::NORMAL);
            }
            else if (res == "GS1") {
                currentGame->SetRevType(ReverbType::GS1);
            }
            else if (res == "GS2") {
                currentGame->SetRevType(ReverbType::GS2);
            }
        }
    }
}
