#pragma once

#include <vector>
#include <string>
#include <filesystem>

#include "GameConfig.h"

class ConfigManager
{
public:
    static ConfigManager& Instance();

    GameConfig& GetCfg();
    const GameConfig& GetCfg() const;
    void SetGameCode(const std::string& gameCode);
    void Load();
    void Save();
    const std::filesystem::path& GetWavOutputDir();
    CGBPolyphony GetCgbPolyphony() const;
    int8_t GetMaxLoopsPlaylist() const;
    int8_t GetMaxLoopsExport() const;
    float GetPadSecondsStart() const;
    float GetPadSecondsEnd() const;
private:
    ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    std::vector<GameConfig> configs;
    std::filesystem::path confWavOutputDir;
    CGBPolyphony confCgbPolyphony;
    int8_t maxLoopsPlaylist;
    int8_t maxLoopsExport;
    std::filesystem::path configPath;
    GameConfig *curCfg = nullptr;
    std::string padSecondsStart;
    std::string padSecondsEnd;
};
