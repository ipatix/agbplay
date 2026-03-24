#include "CLISonglist.hpp"

#include "MP2KContext.hpp"
#include "ProfileManager.hpp"
#include "Settings.hpp"
#include "Xcept.hpp"

#include <cassert>
#include <print>
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

    std::println("Song {} (hex: {:#x}) information:", songIdLL, songIdLL);
    std::println("  Header offset: {:#x}", player.songHeaderPos);
    std::println("  Voice table offset: {:#x}", player.bankPos);
    std::println("  Reverb: {:#x}", player.reverb);
    std::println("  Priority: {}", player.priority);
    std::println("  Player number: {}", player.playerIdx);
    std::println("  Tracks (count: {}):", player.tracksUsed);
    assert(player.tracksUsed <= player.tracks.size());
    for (size_t i = 0; i < player.tracksUsed; i++)
        std::println("  - [{:02d}] {:#x}", i, player.tracks.at(i).pos);
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

    std::println("SongID | Header Off | VoiceT Off | Rev  | Prio | PlyID | #Trks");
    std::println("-------+------------+------------+------+------+-------+------");

    for (size_t i = 0; i < p.songTableInfoPlayback.count; i++) {
        ctx.m4aSongNumStart(static_cast<uint16_t>(i));

        const MP2KPlayer &player = ctx.players.at(ctx.primaryPlayer);

        std::println("{: >6d} | {: >#10x} | {: >#10x} | {:#04x} | {: >4d} | {: >5d} | {: >5d}",
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

    std::println("{}", p.songTableInfoPlayback.count);
}
