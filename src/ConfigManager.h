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
    void SetGameCode(const std::string& gameCode);
    void Load();
    void Save();
    const std::filesystem::path& GetWavOutputDir();
    CGBPolyphony GetCgbPolyphony();
    int8_t GetMaxLoopsPlaylist();
    int8_t GetMaxLoopsExport();
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
};
