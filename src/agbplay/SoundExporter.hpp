#pragma once

#include <cstdint>
#include <filesystem>

struct sf_private_tag;
struct Profile;
struct Settings;

// TODO this class does not really hold useful state, remove class and replace
// with functions only.

class SoundExporter
{
public:
    SoundExporter(
        const std::filesystem::path &directory,
        const Settings &settings,
        const Profile &profile,
        bool benchmarkOnly,
        bool seperate
    );
    SoundExporter(const SoundExporter &) = delete;
    SoundExporter &operator=(const SoundExporter &) = delete;

    void Export();

private:
    void writeSilence(sf_private_tag *ofile, double seconds);
    size_t exportSong(const std::filesystem::path &filePath, uint16_t uid);

    const std::filesystem::path directory;
    const Settings &settings;
    const Profile &profile;

    const bool benchmarkOnly;
    const bool seperate;
};
