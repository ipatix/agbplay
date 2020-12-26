#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <mutex>

#include "SongEntry.h"
#include "GameConfig.h"
#include "ConsoleGUI.h"
#include "SoundData.h"

class SoundExporter
{
public:
    SoundExporter(SongTable& songTable, bool benchmarkOnly, bool seperate);
    SoundExporter(const SoundExporter&) = delete;
    SoundExporter& operator=(const SoundExporter&) = delete;

    void Export(const std::string& outputDir, std::vector<SongEntry>& entries, std::vector<bool>& ticked);
private:
    size_t exportSong(const std::string& fileName, uint16_t uid);

    SongTable& songTable;
    std::mutex uilock;

    bool benchmarkOnly;
    bool seperate; // seperate tracks to multiple files
};
