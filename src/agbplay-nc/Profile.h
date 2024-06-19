#pragma once

#include <vector>
#include <optional>
#include <filesystem>

#include "Types.h"

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

    std::filesystem::path path;
    bool dirty = false;
};
