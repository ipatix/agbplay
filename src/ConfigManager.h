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
    void SetCgbPolyphony(CGBPolyphony value);
    int8_t GetMaxLoopsPlaylist() const;
    void SetMaxLoopsPlaylist(int8_t value);
    int8_t GetMaxLoopsExport() const;
    void SetMaxLoopsExport(int8_t value);
    double GetPadSecondsStart() const;
    void SetPadSecondsStart(double value);
    double GetPadSecondsEnd() const;
    void SetPadSecondsEnd(double value);
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
    double padSecondsStart;
    double padSecondsEnd;
};
