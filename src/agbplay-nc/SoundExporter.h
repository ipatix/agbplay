#pragma once

#include <sndfile.h>
#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>

#include "SongEntry.h"
#include "GameConfig.h"
#include "ConsoleGUI.h"
#include "SoundData.h"

class SoundExporter
{
public:
    SoundExporter(const SongTableInfo &songTableInfo, const PlayerTableInfo &playerTableInfo, bool benchmarkOnly, bool seperate);
    SoundExporter(const SoundExporter&) = delete;
    SoundExporter& operator=(const SoundExporter&) = delete;

    void Export(const std::vector<SongEntry>& entries);
private:
    static void writeSilence(SNDFILE *ofile, double seconds);
    size_t exportSong(const std::filesystem::path& fileName, uint16_t uid);

    const SongTableInfo &songTableInfo;
    const PlayerTableInfo &playerTableInfo;

    bool benchmarkOnly;
    bool seperate; // seperate tracks to multiple files
};
