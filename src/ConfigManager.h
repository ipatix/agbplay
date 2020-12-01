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
    void Save();
private:
    ConfigManager();
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    std::vector<GameConfig> configs;
    std::filesystem::path configPath;
    GameConfig *curCfg = nullptr;
};
