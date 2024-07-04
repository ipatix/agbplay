#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>

#include "Profile.h"
#include "SoundData.h"

struct sf_private_tag;

class SoundExporter
{
public:
    SoundExporter(const Profile &profile, bool benchmarkOnly, bool seperate);
    SoundExporter(const SoundExporter&) = delete;
    SoundExporter& operator=(const SoundExporter&) = delete;

    void Export();
private:
    static void writeSilence(sf_private_tag *ofile, double seconds);
    size_t exportSong(const std::filesystem::path& fileName, uint16_t uid);

    const Profile &profile;

    bool benchmarkOnly;
    bool seperate; // seperate tracks to multiple files
};
