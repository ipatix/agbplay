#include "Render.hpp"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <format>
#include <limits>
#include <string>
#include <string_view>
#include <vector>
#include <fmt/core.h>

#include "ProfileManager.hpp"
#include "Rom.hpp"
#include "Settings.hpp"
#include "SoundExporter.hpp"
#include "Xcept.hpp"

static std::vector<std::string_view> Split(const std::string &input, const std::string &sep)
{
    std::vector<std::string_view> results;

    size_t startPos = 0;
    while (true) {
        size_t endPos = input.find(sep, startPos);
        if (endPos == input.npos)
            endPos = input.size();

        if (endPos > startPos)
            results.emplace_back(input.begin() + startPos, input.begin() + endPos);

        if (endPos >= input.size())
            return results;

        startPos = endPos;
    }
}

void CLI::Render(const std::string &songId, const std::string &outputPath, bool stems)
{
    std::vector<std::string_view> songIds = Split(songId, ";");
    std::vector<std::string_view> outputPaths = Split(outputPath, ";");

    if (songIds.size() == 0)
        return;

    if (outputPath.size() == 0)
        throw Xcept("Cannot render to file. Output path is blank.");

    std::vector<uint16_t> songIdsParsed;
    std::vector<std::filesystem::path> outputPathsParsed;

    // Parse song IDs
    for (size_t i = 0; i < songIds.size(); i++) {
        long long id = std::stoll(std::string(songIds.at(i)));
        if (id < std::numeric_limits<uint16_t>::min() || id > std::numeric_limits<uint16_t>::max())
            throw Xcept("Cannot render song ID, which is not a 16 bit value");
        songIdsParsed.emplace_back(static_cast<uint16_t>(id));
    }

    // Parse output paths
    if (outputPaths.size() == 1) {
        // assert outputPath must be a pattern
        if (songIdsParsed.size() > 1 && outputPaths.back().find(SoundExporter::SONG_ID_PATTERN.string()) == outputPaths.back().npos)
            throw Xcept("Single output path for multiple songs must contain a pattern {}", SoundExporter::SONG_ID_PATTERN.string());

        for (size_t i = 0; i < songIds.size(); i++)
            outputPathsParsed.emplace_back(outputPaths.back());
    } else if (outputPaths.size() == songIds.size()) {
        for (size_t i = 0; i < songIds.size(); i++)
            outputPathsParsed.emplace_back(outputPaths.at(i));
    } else {
        throw Xcept("Song ID count ({}) output path count ({}) mismatch.", songIds.size(), outputPaths.size());
    }

    assert(songIds.size() == outputPathsParsed.size());

    ProfileManager pm;
    pm.LoadProfiles();

    Settings s;
    s.Load();

    // Make a copy of the profile, so we don't destroy the playlist accidentally
    Profile p = *pm.GetCLIDefaultProfile(Rom::Instance());
    p.playlist.clear();
    for (size_t i = 0; i < songIds.size(); i++)
        p.playlist.emplace_back("unnamed song", songIdsParsed.at(i));

    SoundExporter se("", outputPathsParsed, s, p, false, stems);
    se.Export();
}
