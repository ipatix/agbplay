#pragma once

#include <vector>
#include <optional>
#include <filesystem>

#include "Types.h"

/* The Profile struct contains two "Config" and "Playback" states.
 * Only the "Config" state is actually saved to disk.
 * If for example the "Config" uses "auto" values, they should not be written to disk with resolved values.
 * Resolved values from scanning are only used during playback, thus the "Playback" state has to be merged
 * from "Config" and a scan result. */

struct Profile {
    struct PlaylistEntry {
        std::string name;
        uint16_t id = 0;
    };

    struct GameMatch {
        std::vector<std::string> gameCodes;
        std::vector<uint8_t> magicBytes;
    };

    std::vector<PlaylistEntry> playlist;
    SongTableInfo songTableInfoConfig;
    SongTableInfo songTableInfoPlayback;
    PlayerTableInfo playerTableConfig;
    PlayerTableInfo playerTablePlayback;
    MP2KSoundMode mp2kSoundModeConfig;
    MP2KSoundMode mp2kSoundModePlayback;
    AgbplaySoundMode agbplaySoundMode;
    GameMatch gameMatch;
    // sound hooks / patches, TODO
    std::string description;
    // tags, profiles

    std::filesystem::path path;
    bool dirty = false;

    void ApplyScanToPlayback(const SongTableInfo &songTableInfoScan, const PlayerTableInfo &playerTableInfoScan, const MP2KSoundMode &mp2kSoundModeScan);
};