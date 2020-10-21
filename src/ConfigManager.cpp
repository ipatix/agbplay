#include <fstream>
#include <regex>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include <json/json.h>

#include "ConfigManager.h"
#include "Util.h"
#include "Xcept.h"
#include "Debug.h"

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

    Json::Value root;
    configFile >> root;

    if (root["id"].asString() != "agbplay")
        throw Xcept("Bad JSON ID: %s", root["id"].asString().c_str());

    for (Json::Value playlist : root["playlists"]) {
        // parse games
        std::vector<std::string> games;
        for (Json::Value game : playlist["games"])
            games.emplace_back(game.asString());
        configs.emplace_back(games);

        // parse other parameters
        configs.back().SetPCMVol(uint8_t(clip<int>(0, playlist.get("pcm-master-volume", 15).asInt(), 15)));
        configs.back().SetEngineFreq(uint8_t(clip<int>(0, playlist.get("pcm-samplerate", 4).asInt(), 15)));
        configs.back().SetEngineRev(uint8_t(clip<int>(0, playlist.get("pcm-reverb-level", 0).asInt(), 255)));
        configs.back().SetRevBufSize(uint16_t(playlist.get("pcm-reverb-buffer-len", 1536).asUInt()));
        configs.back().SetRevType(str2rev(playlist.get("pcm-reverb-type", "normal").asString()));
        configs.back().SetResType(str2res(playlist.get("pcm-resampling-algo", "linear").asString()));
        configs.back().SetResTypeFixed(str2res(playlist.get("pcm-fixed-rate-resampling-algo", "linear").asString()));
        configs.back().SetTrackLimit(uint8_t(clip<int>(0, playlist.get("song-track-limit", 16).asInt(), 16)));

        for (Json::Value song : playlist["songs"]) {
            configs.back().GetGameEntries().emplace_back(
                    song.get("name", "?").asString(),
                    static_cast<uint16_t>(song.get("index", "0").asUInt()));
        }
    }

    curCfg = nullptr;
}

ConfigManager::~ConfigManager()
{
}

ConfigManager& ConfigManager::Instance()
{
    static ConfigManager cm("agbplay.json");
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
    for (GameConfig& config : configs)
    {
        const auto &gameCodesToCheck = config.GetGameCodes();
        if (std::find(gameCodesToCheck.begin(), gameCodesToCheck.end(), gameCode) != gameCodesToCheck.end()) {
            curCfg = &config;
            return;
        }
    }
    configs.emplace_back(gameCode);
    curCfg = &configs.back();
}

void ConfigManager::Save()
{
    Json::Value playlists;
    for (GameConfig& cfg : configs)
    {
        Json::Value playlist;
        playlist["pcm-master-volume"] = static_cast<int>(cfg.GetPCMVol());
        playlist["pcm-samplerate"] = static_cast<int>(cfg.GetEngineFreq());
        playlist["pcm-reverb-level"] = static_cast<int>(cfg.GetEngineRev());
        playlist["pcm-reverb-buffer-len"] = static_cast<int>(cfg.GetRevBufSize());
        playlist["pcm-reverb-type"] = rev2str(cfg.GetRevType());
        playlist["pcm-resampling-algo"] = res2str(cfg.GetResType());
        playlist["pcm-fixed-rate-resampling-algo"] = res2str(cfg.GetResTypeFixed());
        playlist["song-track-limit"] = static_cast<int>(cfg.GetTrackLimit());

        Json::Value games;
        for (const std::string& code : cfg.GetGameCodes())
            games.append(code);
        playlist["games"] = games;

        Json::Value songs;
        for (SongEntry entr : cfg.GetGameEntries()) {
            Json::Value song;
            song["index"] = entr.GetUID();
            song["name"] = entr.name;
            songs.append(song);
        }
        playlist["songs"] = songs;
        playlists.append(playlist);
    }

    Json::Value root;
    root["id"] = "agbplay";
    root["playlists"] = playlists;

    ofstream jsonFile(configPath);
    if (!jsonFile.is_open())
        throw Xcept("Error while writing agbplay.json: %s", strerror(errno));

    jsonFile << root << std::endl;

    _print_debug("Configuration/Playlist saved!");
}
