#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <mutex>

#include "StreamGenerator.h"
#include "SongEntry.h"
#include "GameConfig.h"
#include "ConsoleGUI.h"

class SoundExporter
{
public:
    SoundExporter(SoundData& _sd, bool _benchmarkOnly, bool seperate);
    SoundExporter(const SoundExporter&) = delete;
    SoundExporter& operator=(const SoundExporter&) = delete;

    void Export(const std::string& outputDir, std::vector<SongEntry>& entries, std::vector<bool>& ticked);
private:
    size_t exportSong(const std::string& fileName, uint16_t uid);

    SoundData& sd;
    std::mutex uilock;

    bool benchmarkOnly;
    bool seperate; // seperate tracks to multiple files
};
