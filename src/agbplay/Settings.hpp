#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>

struct Settings
{
    void Load();
    void Save();

    uint32_t playbackSampleRate = 0;
    uint32_t playbackOutputNumBuffers = 0;

    uint32_t exportSampleRate = 0;
    uint32_t exportBitDepth = 0;
    double exportPadStart = 0.0;
    double exportPadEnd = 0.0;
    std::filesystem::path exportQuickExportDirectory;
    bool exportQuickExportAsk = false;

    std::filesystem::path lastOpenFileDirectory;
};
