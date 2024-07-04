#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>

#include "Types.h"

class Rom;

class MP2KScanner {
public:
    MP2KScanner(const Rom &rom);

    struct Result {
        MP2KSoundMode mp2kSoundMode;
        PlayerTableInfo playerTableInfo;
        SongTableInfo songTableInfo;
    };

    std::vector<Result> Scan();

private:
    bool FindSongTable(size_t &findStartPos, size_t &songTablePos, uint16_t &songCount) const;
    bool FindPlayerTable(size_t songTablePos, size_t &playerTablePos, PlayerTableInfo &playerTableInfo) const;
    bool FindSoundMode(size_t playerTablePos, size_t &soundModePos, uint32_t &soundMode) const;

    bool IsPosReferenced(size_t pos) const;
    bool IsPosReferenced(size_t pos, size_t &findStartPos, size_t &referencePos) const;
    bool IsPosReferenced(const std::vector<size_t> &poss, size_t &index) const;
    bool IsValidSongTableEntry(size_t pos) const;
    bool IsValidPlayerTableEntry(size_t pos) const;

    static bool IsValidIwramPointer(uint32_t word);
    static bool IsValidEwramPointer(uint32_t word);
    static bool IsValidRamPointer(uint32_t word);

    static const size_t MIN_SONG_NUM = 4; // lowest one seen is 7 in Tetris World
    static const size_t SEARCH_START = 0x200;

    const Rom &rom;
};
