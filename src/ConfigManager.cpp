#include <fstream>
#include <regex>
#include <cstring>
#include <iostream>
#include <cstdlib>

#include "ConfigManager.h"
#include "Util.h"
#include "Xcept.h"

using namespace std;
using namespace agbplay;

ConfigManager::ConfigManager(const string& configPath)
{
    this->configPath = configPath;
    // parse things from config file
    ifstream configFile(configPath);
    if (!configFile.is_open()) {
        throw Xcept("Error while opening config file: %s", strerror(errno));
    }
    string line;
    GameConfig *currentGame = nullptr;
    regex gameExpr("^\\s*\\[([0-9A-Z]{4})\\]\\s*$");
    regex songExpr("^\\s*(\\d+)\\s*=\\s*\\b(.+)\\b\\s*$");
    regex cfgVolExpr("^\\s*ENG_VOL\\s*=\\s*(\\d+)\\s*$");
    regex cfgFreqExpr("^\\s*ENG_FREQ\\s*=\\s*(\\d+)\\s*$");
    regex cfgRevExpr("^\\s*ENG_REV\\s*=\\s*(\\d+)\\s*$");
    regex cfgRevTypeExpr("^\\s*ENG_REV_TYPE\\s*=\\s*(.*)\\s*$");
    regex cfgTrackLimitExpr("^\\s*TRACK_LIMIT\\s*=\\s*(\\d+)\\s*$");

    while (getline(configFile, line)) {
        if (configFile.bad()) {
            throw Xcept("Error while reading config file: %s", strerror(errno));
        }
        smatch sm;
        if (regex_match(line, sm, songExpr) && sm.size() == 3 && currentGame != nullptr) {
            currentGame->GetGameEntries().push_back(SongEntry(sm[2], (uint16_t(stoi(sm[1])))));
        }
        else if (regex_match(line, sm, gameExpr) && sm.size() == 2) {
            currentGame = &GetConfig(sm[1]);
        }
        else if (regex_match(line, sm, cfgVolExpr) && sm.size() == 2 && currentGame != nullptr) {
            currentGame->SetPCMVol(uint8_t(clip<int>(0, stoi(sm[1]), 15)));
        }
        else if (regex_match(line, sm, cfgFreqExpr) && sm.size() == 2 && currentGame != nullptr) {
            currentGame->SetEngineFreq(uint8_t(clip<int>(0, stoi(sm[1]), 15)));
        }
        else if (regex_match(line, sm, cfgRevExpr) && sm.size() == 2 && currentGame != nullptr) {
            currentGame->SetEngineRev(uint8_t(clip<int>(0, stoi(sm[1]), 255)));
        }
        else if (regex_match(line, sm, cfgRevTypeExpr) && sm.size() == 2 && currentGame != nullptr) {
            string res = sm[1];
            if (res == "NORMAL") {
                currentGame->SetRevType(ReverbType::NORMAL);
            }
            else if (res == "GS1") {
                currentGame->SetRevType(ReverbType::GS1);
            }
            else if (res == "GS2") {
                currentGame->SetRevType(ReverbType::GS2);
            }
            else if (res == "MGAT") {
                currentGame->SetRevType(ReverbType::MGAT);
            }
            else if (res == "TEST") {
                currentGame->SetRevType(ReverbType::TEST);
            }
        }
        else if (regex_match(line, sm, cfgTrackLimitExpr) && sm.size() == 2 && currentGame != nullptr) {
            currentGame->SetTrackLimit(uint8_t(clip<int>(0, stoi(sm[1]), 16)));
        }
    }
}

ConfigManager::~ConfigManager()
{
    ofstream configFile(configPath);
    if (!configFile.is_open()) {
        std::cerr << "Error while writing config file: " << strerror(errno) << std::endl;
        abort();
    }
    for (GameConfig& cfg : configs)
    {
        configFile << FormatString("[%s]", cfg.GetGameCode().c_str()) << endl;
        configFile << FormatString("ENG_VOL = %d", (int)cfg.GetPCMVol()) << endl;
        configFile << FormatString("ENG_FREQ = %d", (int)cfg.GetEngineFreq()) << endl;
        configFile << FormatString("ENG_REV = %d", (int)cfg.GetEngineRev()) << endl;
        configFile << "ENG_REV_TYPE = ";
        switch (cfg.GetRevType()) {
            case ReverbType::NORMAL:
                configFile << "NORMAL";
                break;
            case ReverbType::GS1:
                configFile << "GS1";
                break;
            case ReverbType::GS2:
                configFile << "GS2";
                break;
            case ReverbType::MGAT:
                configFile << "MGAT";
                break;
            case ReverbType::TEST:
                configFile << "TEST";
                break;
        }
        configFile << endl;
        configFile << FormatString("TRACK_LIMIT = %d", (int)cfg.GetTrackLimit()) << endl;
        for (SongEntry entr : cfg.GetGameEntries())
        {
            configFile << FormatString("%04d = %s", (int)entr.GetUID(), entr.name.c_str()) << endl;
        }
    }
}

GameConfig& ConfigManager::GetConfig(const string& gameCode)
{
    for (GameConfig& game : configs)
    {
        if (game.GetGameCode() == gameCode) {
            return game;
        }
    }
    configs.emplace_back(gameCode);
    return configs[configs.size() - 1];
}
