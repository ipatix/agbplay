#pragma once

#include <vector>

struct Profile {
    std::vector<PlaylistEntry> playlist;
    size_t songTableId; // abstract this away in its own struct if there are more scanner hints
    GameMatch gameMatch;
    MP2KSoundMode mp2kSoundMode;
    AgbplaySoundMode agbplaySoundMode;
    // sound hooks / patches, TODO
};
