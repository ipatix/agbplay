#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>

class Settings
{
public:
    void Load();
    void Save();

    static void CreateInstance(const std::filesystem::path &filePath);
    static Settings &Instance() { return *globalInstance; }

    uint32_t playbackSampleRate = 0;

    uint32_t exportSampleRate = 0;
    std::filesystem::path exportQuickExportDirectory;
    bool exportQuickExportAsk = false;

private:
    static std::unique_ptr<Settings> globalInstance;
};
