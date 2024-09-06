#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>

#include "Profile.hpp"
#include "SoundData.hpp"

struct sf_private_tag;

// TODO this class does not really hold useful state, remove class and replace
// with functions only.

class SoundExporter
{
public:
    SoundExporter(const std::filesystem::path &directory, const Profile &profile, bool benchmarkOnly, bool seperate);
    SoundExporter(const SoundExporter&) = delete;
    SoundExporter& operator=(const SoundExporter&) = delete;

    static std::filesystem::path DefaultDirectory();
    void Export();
private:
    static void writeSilence(sf_private_tag *ofile, double seconds);
    size_t exportSong(const std::filesystem::path& filePath, uint16_t uid);

    const std::filesystem::path directory;
    const Profile &profile;

    const bool benchmarkOnly;
    const bool seperate;
};
