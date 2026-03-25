#include "CLISonglist.hpp"

#include "MP2KContext.hpp"
#include "ProfileManager.hpp"
#include "Settings.hpp"
#include "Xcept.hpp"

#include <cassert>
#include <fmt/core.h>
#include <limits>
#include <string>

void CLI::SonglistShow(const std::string &songId)
{
    const long long songIdLL = std::stoll(songId);
    if (songIdLL > std::numeric_limits<uint16_t>::max() || songIdLL < std::numeric_limits<uint16_t>::min())
        throw Xcept("song-id is out of u16 range: {}", songIdLL);

    ProfileManager pm;
    pm.LoadProfiles();

    Settings s;
    s.Load();

    Profile p = *pm.GetCLIDefaultProfile(Rom::Instance());

    MP2KContext ctx(
        s.exportSampleRate,
        s.exportMaxLoops,
        Rom::Instance(),
        p.mp2kSoundModePlayback,
        p.agbplaySoundMode,
        p.songTableInfoPlayback,
        p.playerTablePlayback
    );

    ctx.m4aSongNumStart(static_cast<uint16_t>(songIdLL));

    const MP2KPlayer &player = ctx.players.at(ctx.primaryPlayer);

    fmt::print("Song {} (hex: {:#x}) information:\n", songIdLL, songIdLL);
    fmt::print("  Header offset: {:#x}\n", player.songHeaderPos);
    fmt::print("  Voice table offset: {:#x}\n", player.bankPos);
    fmt::print("  Reverb: {:#x}\n", player.reverb);
    fmt::print("  Priority: {}\n", player.priority);
    fmt::print("  Player number: {}\n", player.playerIdx);
    fmt::print("  Tracks (count: {}):\n", player.tracksUsed);
    assert(player.tracksUsed <= player.tracks.size());
    for (size_t i = 0; i < player.tracksUsed; i++)
        fmt::print("  - [{:02d}] {:#x}\n", i, player.tracks.at(i).pos);
}

void CLI::SonglistList()
{
    ProfileManager pm;
    pm.LoadProfiles();

    Settings s;
    s.Load();

    Profile p = *pm.GetCLIDefaultProfile(Rom::Instance());

    MP2KContext ctx(
        s.exportSampleRate,
        s.exportMaxLoops,
        Rom::Instance(),
        p.mp2kSoundModePlayback,
        p.agbplaySoundMode,
        p.songTableInfoPlayback,
        p.playerTablePlayback
    );

    assert(!p.songTableInfoPlayback.IsAuto());

    fmt::print("SongID | Header Off | VoiceT Off | Rev  | Prio | PlyID | #Trks\n");
    fmt::print("-------+------------+------------+------+------+-------+------\n");

    for (size_t i = 0; i < p.songTableInfoPlayback.count; i++) {
        ctx.m4aSongNumStart(static_cast<uint16_t>(i));

        const MP2KPlayer &player = ctx.players.at(ctx.primaryPlayer);

        fmt::print("{: >6d} | {: >#10x} | {: >#10x} | {:#04x} | {: >4d} | {: >5d} | {: >5d}\n",
            i,
            player.songHeaderPos,
            player.bankPos,
            player.reverb,
            player.priority,
            player.playerIdx,
            player.tracksUsed
        );
    }
}

void CLI::SonglistCount()
{
    ProfileManager pm;
    pm.LoadProfiles();

    Profile p = *pm.GetCLIDefaultProfile(Rom::Instance());

    assert(!p.songTableInfoPlayback.IsAuto());

    fmt::print("{}\n", p.songTableInfoPlayback.count);
}
