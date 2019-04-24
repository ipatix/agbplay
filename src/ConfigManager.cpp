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
    regex gameExpr("^\\s*\\[([0-9A-Z]{4})\\]\\s*$");
    regex songExpr("^\\s*(\\d+)\\s*=\\s*\\b(.+)\\b\\s*$");
    regex cfgVolExpr("^\\s*ENG_VOL\\s*=\\s*(\\d+)\\s*$");
    regex cfgFreqExpr("^\\s*ENG_FREQ\\s*=\\s*(\\d+)\\s*$");
    regex cfgRevExpr("^\\s*ENG_REV\\s*=\\s*(\\d+)\\s*$");
    regex cfgRevTypeExpr("^\\s*ENG_REV_TYPE\\s*=\\s*(.*)\\s*$");
    regex cfgTrackLimitExpr("^\\s*TRACK_LIMIT\\s*=\\s*(\\d+)\\s*$");
    regex cfgPcmRes("^\\s*PCM_RES_TYPE\\s*=\\s*(.*)\\s*$");
    regex cfgPcmFixedRes("^\\s*PCM_FIX_RES_TYPE\\s*=\\s*(.*)\\s*$");
    regex cfgRevBufSize("^\\s*REV_BUF_SIZE\\s*=\\s*0x(\\d+)\\s*$");

    while (getline(configFile, line)) {
        if (configFile.bad()) {
            throw Xcept("Error while reading config file: %s", strerror(errno));
        }
        smatch sm;
        if (regex_match(line, sm, songExpr) && sm.size() == 3 && curCfg) {
            curCfg->GetGameEntries().push_back(SongEntry(sm[2], (uint16_t(stoi(sm[1])))));
        }
        else if (regex_match(line, sm, gameExpr) && sm.size() == 2) {
            // set's curCfg
            SetGameCode(sm[1]);
        }
        else if (regex_match(line, sm, cfgVolExpr) && sm.size() == 2 && curCfg) {
            curCfg ->SetPCMVol(uint8_t(clip<int>(0, stoi(sm[1]), 15)));
        }
        else if (regex_match(line, sm, cfgFreqExpr) && sm.size() == 2 && curCfg) {
            curCfg->SetEngineFreq(uint8_t(clip<int>(0, stoi(sm[1]), 15)));
        }
        else if (regex_match(line, sm, cfgRevExpr) && sm.size() == 2 && curCfg) {
            curCfg->SetEngineRev(uint8_t(clip<int>(0, stoi(sm[1]), 255)));
        }
        else if (regex_match(line, sm, cfgRevTypeExpr) && sm.size() == 2 && curCfg) {
            curCfg->SetRevType(str2rev(sm[1]));
        }
        else if (regex_match(line, sm, cfgTrackLimitExpr) && sm.size() == 2 && curCfg) {
            curCfg->SetTrackLimit(uint8_t(clip<int>(0, stoi(sm[1]), 16)));
        }
        else if (regex_match(line, sm, cfgPcmRes) && sm.size() == 2 && curCfg) {
            curCfg->SetResType(str2res(sm[1]));
        }
        else if (regex_match(line, sm, cfgPcmFixedRes) && sm.size() == 2 && curCfg) {
            curCfg->SetResTypeFixed(str2res(sm[1]));
        }
        else if (regex_match(line, sm, cfgRevBufSize) && curCfg) {
            curCfg->SetRevBufSize(uint16_t(stoul(sm[1], NULL, 16)));
	    }
    }

    curCfg = nullptr;
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
        configFile << "[" << cfg.GetGameCode() << "]" << endl;
        configFile << "ENG_VOL = " << static_cast<int>(cfg.GetPCMVol()) << endl;
        configFile << "ENG_FREQ = " << static_cast<int>(cfg.GetEngineFreq()) << endl;
        configFile << "ENG_REV = " << static_cast<int>(cfg.GetEngineRev()) << endl;
        configFile << "ENG_REV_TYPE = " << rev2str(cfg.GetRevType()) << endl;
        configFile << "PCM_RES_TYPE = " << res2str(cfg.GetResType()) << endl;
        configFile << "PCM_FIX_RES_TYPE = " << res2str(cfg.GetResTypeFixed()) << endl;
        configFile << "TRACK_LIMIT = " << static_cast<int>(cfg.GetTrackLimit()) << endl;
	    configFile << "REV_BUF_SIZE = 0x" << std::uppercase << std::hex << (cfg.GetRevBufSize()) << endl << std::dec << std::nouppercase;


        for (SongEntry entr : cfg.GetGameEntries()) {
            char oldFill = configFile.fill('0');
            streamsize oldWidth = configFile.width(4);

            configFile << static_cast<int>(entr.GetUID());

            configFile.width(oldWidth);
            configFile.fill(oldFill);

            configFile << " = " << entr.name << endl;
        }

    }
}

ConfigManager& ConfigManager::Instance()
{
    static ConfigManager cm("agbplay.ini");
    return cm;
}

GameConfig& ConfigManager::GetCfg()
{
    if (curCfg)
        return *curCfg;
    else
        throw Xcept("Trying to get the game config without setting the game code");
}

void ConfigManager::SetGameCode(const std::string& gameCode)
{
    for (GameConfig& game : configs)
    {
        if (game.GetGameCode() == gameCode) {
            curCfg = &game;
            return;
        }
    }
    configs.emplace_back(gameCode);
    curCfg = &configs[configs.size() - 1];
}
