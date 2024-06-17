#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>

class Rom;

class MP2KScanner {
public:
    MP2KScanner(const Rom &rom);

    struct Result {
        uint8_t pcm_vol = 0;
        uint8_t pcm_rev = 0;
        uint8_t pcm_freq = 0;
        uint8_t pcm_max_channels = 0;   // not used in agbplay
        uint8_t dac_config = 0;         // not used in agbplay

        std::vector<std::pair<uint8_t, bool>> player_configs;

        size_t songtable_pos = 0;
        uint16_t song_count = 0;
    };

    std::vector<Result> Scan();

private:
    bool FindSongTable(size_t &findStartPos, size_t &songTablePos, uint16_t &songCount) const;
    bool FindPlayerTable(size_t songTablePos, size_t &playerTablePos, std::vector<std::pair<uint8_t, bool>> &playerConfigs) const;
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
