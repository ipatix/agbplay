#include <fstream>
#include <regex>
#include <cstring>
#include <iostream>
#include <cstdlib>

// sorry, for some reason multiple versions of jsoncpp use different paths :/
#if __has_include(<json/json.h>)
#include <json/json.h>
#include <json/writer.h>
#else
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/writer.h>
#endif

#include "ConfigManager.h"
#include "Util.h"
#include "Xcept.h"
#include "Debug.h"
#include "OS.h"

ConfigManager& ConfigManager::Instance()
{
    static ConfigManager cm;
    return cm;
}

GameConfig& ConfigManager::GetCfg()
{
    if (curCfg)
        return *curCfg;
    else
        throw Xcept("Trying to get the game config without setting the game code");
}

const GameConfig& ConfigManager::GetCfg() const
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

void ConfigManager::Load()
{
    configPath = OS::GetLocalConfigDirectory() / "agbplay.json";
    const auto globalConfigPath = OS::GetGlobalConfigDirectory() / "agbplay.json";

    /* Parse things from config file.
     * If the config file in home directory is not found,
     * try reading it from /etc/agbplay/agbplay.json.
     * If this isn't found either, use an empty config file. */
    Json::Value root;
    if (std::ifstream configFile; configFile.open(configPath), configFile.is_open()) {
        Debug::print("User local configuration found!");
        configFile >> root;
    } else if (configFile.open(globalConfigPath); configFile.is_open()) {
        Debug::print("Global configuration found!");
        configFile >> root;
    } else {
        Debug::print("No configuration file found. Creating new configuration.");
        root["id"] = "agbplay";
        root["playlists"] = Json::Value();  // null value
    }

    if (root["id"].asString() != "agbplay")
        throw Xcept("Bad JSON ID: %s", root["id"].asString().c_str());

    // output directory used for saving rendered sogs
    if (root.isMember("wav-output-dir")) {
        confWavOutputDir = root["wav-output-dir"].asString();
    } else {
        confWavOutputDir = OS::GetMusicDirectory() / "agbplay";
    }

    // CGB channel polyphony configuration
    confCgbPolyphony = str2cgbPoly(root.get("cgb-polyphony", "mono-strict").asString());
    
    // Loop configuration
    maxLoopsPlaylist = static_cast<int8_t>(root.get("max-loops-playlist", 1).asInt());
    maxLoopsExport = static_cast<int8_t>(root.get("max-loops-export", 1).asInt());

    // Silence padding
    padSecondsStart = root.get("pad-seconds-start", 0.0).asDouble();
    padSecondsEnd = root.get("pad-seconds-end", 0.0).asDouble();

    for (Json::Value playlist : root["playlists"]) {
        // parse games
        std::vector<std::string> games;
        for (Json::Value game : playlist["games"])
            games.emplace_back(game.asString());
        configs.emplace_back(games);

        // parse other parameters
        configs.back().SetPCMVol(uint8_t(std::clamp<int>(playlist.get("pcm-master-volume", 15).asInt(), 0, 15)));
        configs.back().SetEngineFreq(uint8_t(std::clamp<int>(playlist.get("pcm-samplerate", 4).asInt(), 0, 15)));
        configs.back().SetEngineRev(uint8_t(std::clamp<int>(playlist.get("pcm-reverb-level", 0).asInt(), 0, 255)));
        configs.back().SetRevBufSize(uint16_t(playlist.get("pcm-reverb-buffer-len", 0x630).asUInt()));
        configs.back().SetRevType(str2rev(playlist.get("pcm-reverb-type", "normal").asString()));
        configs.back().SetResType(str2res(playlist.get("pcm-resampling-algo", "linear").asString()));
        configs.back().SetResTypeFixed(str2res(playlist.get("pcm-fixed-rate-resampling-algo", "linear").asString()));
        configs.back().SetTrackLimit(uint8_t(std::clamp<int>(playlist.get("song-track-limit", 16).asInt(), 0, 16)));
        configs.back().SetAccurateCh3Volume(playlist.get("accurate-ch3-volume", false).asBool());
        configs.back().SetAccurateCh3Quantization(playlist.get("accurate-ch3-quantization", false).asBool());

        for (Json::Value song : playlist["songs"]) {
            configs.back().GetGameEntries().emplace_back(
                    song.get("name", "?").asString(),
                    static_cast<uint16_t>(song.get("index", "0").asUInt()));
        }
    }
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
        playlist["accurate-ch3-volume"] = cfg.GetAccurateCh3Volume();
        playlist["accurate-ch3-quantization"] = cfg.GetAccurateCh3Quantization();

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
    root["wav-output-dir"] = confWavOutputDir.string();
    root["cgb-polyphony"] = cgbPoly2str(confCgbPolyphony);
    root["playlists"] = playlists;
    root["max-loops-playlist"] = maxLoopsPlaylist;
    root["max-loops-export"] = maxLoopsExport;
    root["pad-seconds-start"] = padSecondsStart;
    root["pad-seconds-end"] = padSecondsEnd;

    std::filesystem::create_directories(configPath.parent_path());
    std::ofstream jsonFile(configPath);
    if (!jsonFile.is_open())
        throw Xcept("Error while writing agbplay.json: %s", strerror(errno));

    Json::StreamWriterBuilder builder;
    builder["emitUTF8"] = true;
    builder["commentStyle"] = "None";   // <-- this prevents trailing whitespaces in all versions
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    writer->write(root, &jsonFile);
    jsonFile << std::endl;

    Debug::print("Configuration/Playlist saved!");
}

const std::filesystem::path& ConfigManager::GetWavOutputDir()
{
    return confWavOutputDir;
}

CGBPolyphony ConfigManager::GetCgbPolyphony() const
{
    return confCgbPolyphony;
}

int8_t ConfigManager::GetMaxLoopsPlaylist() const
{
    return maxLoopsPlaylist < -1 ? 0 : maxLoopsPlaylist;
}

int8_t ConfigManager::GetMaxLoopsExport() const
{
    return maxLoopsExport < 0 ? 0 : maxLoopsExport;
}

double ConfigManager::GetPadSecondsStart() const
{
    return padSecondsStart;
}

double ConfigManager::GetPadSecondsEnd() const
{
    return padSecondsEnd;
}
