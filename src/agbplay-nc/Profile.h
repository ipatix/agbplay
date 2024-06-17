#pragma once

#include <vector>
#include <optional>

#include "Types.h"

struct Profile {
    struct PlaylistEntry {
        std::string name;
        uint16_t id;
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
};
