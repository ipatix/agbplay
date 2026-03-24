#include "Playlist.hpp"

#include <fmt/core.h>

#include "ProfileManager.hpp"
#include "Rom.hpp"
#include "Xcept.hpp"

void CLI::PlaylistList()
{
    ProfileManager pm;
    pm.LoadProfiles();

    Profile p = *pm.GetCLIDefaultProfile(Rom::Instance());

    fmt::print("Index | SongID | Name\n");
    fmt::print("------+--------+-----\n");

    for (size_t i = 0; i < p.playlist.size(); i++) {
        const auto &entry = p.playlist.at(i);
        fmt::print("{: >5d} | {: >6d} | {}\n", i, entry.id, entry.name);
    }
}

void CLI::PlaylistCount()
{
    ProfileManager pm;
    pm.LoadProfiles();

    Profile p = *pm.GetCLIDefaultProfile(Rom::Instance());

    fmt::print("{}\n", p.playlist.size());
}

void CLI::PlaylistSongAdd(const std::string &name, const std::string &songId, const std::string &playlistIdx)
{
    const long long songIdLL = std::stoll(songId);
    if (songIdLL > std::numeric_limits<uint16_t>::max() || songIdLL < std::numeric_limits<uint16_t>::min())
        throw Xcept("song-id is out of u16 range: {}", songIdLL);

    const long long playlistIdxLL = std::stoll(playlistIdx);
    if (playlistIdxLL < 0)
        throw Xcept("playlist-idx must not be negative");
    const size_t playlistIdxSz = static_cast<size_t>(playlistIdxLL);

    ProfileManager pm;
    pm.LoadProfiles();

    auto p = pm.GetCLIDefaultProfile(Rom::Instance());

    if (playlistIdxSz > p->playlist.size())
        throw Xcept("Cannot add entry at position {}, which is beyond end {}", playlistIdxSz, p->playlist.size());

    p->playlist.emplace(p->playlist.begin() + playlistIdxSz, name, static_cast<uint16_t>(songIdLL));
    p->dirty = true;

    pm.SaveProfiles();
}

void CLI::PlaylistSongRemove(const std::string &playlistIdx)
{
    const long long playlistIdxLL = std::stoll(playlistIdx);
    if (playlistIdxLL < 0)
        throw Xcept("playlist-idx must not be negative");
    const size_t playlistIdxSz = static_cast<size_t>(playlistIdxLL);

    ProfileManager pm;
    pm.LoadProfiles();

    auto p = pm.GetCLIDefaultProfile(Rom::Instance());

    if (playlistIdxSz >= p->playlist.size())
        throw Xcept("Cannot remove entry at position {}, which is beyond end {}", playlistIdxSz, p->playlist.size());

    p->playlist.erase(p->playlist.begin() + playlistIdxSz);
    p->dirty = true;

    pm.SaveProfiles();
}
