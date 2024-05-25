#pragma once

#include <vector>
#include <bitset>

#include "Types.h"

const uint8_t BANKDATA_TYPE_SPLIT = 0x40;
const uint8_t BANKDATA_TYPE_RHYTHM = 0x80;

const uint8_t BANKDATA_TYPE_CGB = 0x07;
const uint8_t BANKDATA_TYPE_FIX = 0x08;

const uint8_t BANKDATA_TYPE_PCM = 0x00;
const uint8_t BANKDATA_TYPE_SQ1 = 0x01;
const uint8_t BANKDATA_TYPE_SQ2 = 0x02;
const uint8_t BANKDATA_TYPE_WAVE = 0x03;
const uint8_t BANKDATA_TYPE_NOISE = 0x04;

class SongTable
{
public:
    static std::vector<SongTable> ScanForTables();
    SongTable(size_t songTablePos);
    SongTable(const SongTable&) = default;
    SongTable& operator=(const SongTable&) = default;

    size_t GetSongTablePos();
    size_t GetPosOfSong(uint16_t uid);
    size_t GetNumSongs();
private:
    static bool validateTableEntry(size_t pos);
    static bool validateSong(size_t songPos);
    size_t determineNumSongs() const;

    size_t songTablePos;
    size_t numSongs;
};
