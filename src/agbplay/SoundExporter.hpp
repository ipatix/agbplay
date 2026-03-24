#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

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
        const std::vector<std::filesystem::path> filePaths,
        const Settings &settings,
        const Profile &profile,
        bool benchmarkOnly,
        bool seperate
    );
    SoundExporter(const SoundExporter &) = delete;
    SoundExporter &operator=(const SoundExporter &) = delete;

    void Export();

    static const std::filesystem::path SONG_NAME_PATTERN;
    static const std::filesystem::path TRACK_ID_PATTERN;
    static const std::filesystem::path SONG_ID_PATTERN;

private:
    void writeSilence(sf_private_tag *ofile, double seconds);
    size_t exportSong(const std::filesystem::path &filePathPatt, size_t playlistIndex);
    std::filesystem::path makeFilePath(const std::filesystem::path &filePathPatt, size_t playlistIndex, size_t trackId = 0);

    const std::filesystem::path directory;
    const std::vector<std::filesystem::path> filePaths;
    const Settings &settings;
    const Profile &profile;

    const bool benchmarkOnly;
    const bool seperate;
};
