#include <cstring>
#include <iostream>
#include <cassert>
#include <algorithm>

#include "AgbTypes.h"
#include "SoundData.h"
#include "Xcept.h"
#include "Debug.h"
#include "Util.h"
#include "Rom.h"

/*
 * public Sequence
 */

Sequence::Sequence(uint8_t trackLimit)
    : memaccArea(256), trackLimit(trackLimit)
{
    Init(0);
}

void Sequence::Init(size_t songHeaderPos)
{
    Rom& rom = Rom::Instance();

    this->songHeaderPos = songHeaderPos;

    tracks.clear();
    if (songHeaderPos != 0) {
        // read song header
        size_t nTracks = std::min<uint8_t>(rom.ReadU8(songHeaderPos + 0), trackLimit);

        // read track pointers
        for (size_t i = 0; i < nTracks; i++)
            tracks.emplace_back(rom.ReadAgbPtrToPos(songHeaderPos + 8 + 4 * i));
    }

    // reset runtime variables
    bpmStack = 0;
    bpm = 150;
    tickCount = 0;
}

void Sequence::Reset()
{
    Init(songHeaderPos);
}

size_t Sequence::GetSoundBankPos() const
{
    if (songHeaderPos == 0)
        return 0;

    Rom& rom = Rom::Instance();
    /* Sometimes songs have 0 tracks and will not have a valid
     * sound bank pointer. Return a dummy result instead since
     * it should not be accessed anyway */
    if (!rom.ValidPointer(rom.ReadU32(songHeaderPos + 4)))
        return 0;

    return rom.ReadAgbPtrToPos(songHeaderPos + 4);
}

uint8_t Sequence::GetReverb() const
{
    if (songHeaderPos == 0)
        return 0;

    return Rom::Instance().ReadU8(songHeaderPos + 3);
}

uint8_t Sequence::GetPriority() const
{
    if (songHeaderPos == 0)
        return 0;

    return Rom::Instance().ReadU8(songHeaderPos + 2);
}

size_t Sequence::GetSongHeaderPos() const
{
    return songHeaderPos;
}

/*
 * public
 * Track
 */

Track::Track(size_t pos)
    : pos(pos)
{
}

int16_t Track::GetPitch()
{
    int p = tune + bend * bendr + keyShift * 64;
    if (modt == MODT::PITCH)
        p += lfoValue * 4;
    return static_cast<int16_t>(p);
}

uint16_t Track::GetVol()
{
    int32_t v = vol << 1;
    if (modt == MODT::VOL)
        v = (v * (lfoValue + 128)) >> 7;
    return static_cast<uint16_t>(v);
}

int16_t Track::GetPan()
{
    int p = pan << 1;
    if (modt == MODT::PAN)
        p += lfoValue;
    return static_cast<int16_t>(p);
}

void Track::ResetLfoValue()
{
    lfoValue = 0;
    lfoPhase = 0;

    if (modt == MODT::PITCH)
        updatePitch = true;
    else
        updateVolume = true;
}

/*
 * public
 * SongTable
 */

std::vector<SongTable> SongTable::ScanForTables()
{
    Rom& rom = Rom::Instance();
    std::vector<SongTable> tables;

    for (size_t i = 0x200; i < rom.Size(); i += 4) {
        bool validEntries = true;
        size_t location = i;
        size_t j = 0;
        for (j = 0; j < MIN_SONG_NUM; j++) {
            if (!validateTableEntry(i + j * 8)) {
                i += j * 8;
                validEntries = false;
                break;
            }
        }
        if (validEntries) {
            // before returning, check if reference to song table exists
            for (size_t k = 0x200; k < rom.Size() - 3; k += 4) { // -3 due to possible alignment issues
                size_t value = rom.ReadU32(k) - AGB_MAP_ROM;
                if (value == location) {
                    SongTable songTable(location);
                    j = songTable.GetNumSongs();
                    tables.push_back(songTable);
                    break;
                }
            }
            i += j * 8;
        }
    }

    if (tables.size() == 0) {
      throw Xcept("Unable to find songtable");
    }

    return tables;
}

SongTable::SongTable(size_t songTablePos)
    : songTablePos(songTablePos)
{
    numSongs = determineNumSongs();
}

size_t SongTable::GetSongTablePos() {
    return songTablePos;
}

size_t SongTable::GetPosOfSong(uint16_t uid) {
    return Rom::Instance().ReadAgbPtrToPos(songTablePos + uid * 8);
}

size_t SongTable::GetNumSongs() {
    return numSongs;
}

/*
 * private
 */

bool SongTable::validateTableEntry(size_t pos)
{
    Rom& rom = Rom::Instance();

    // check if the pointer is actually valid
    uint32_t songPtr = rom.ReadU32(pos);
    if (!rom.ValidPointer(songPtr))
        return false;

    // check if the song groups are set appropriately
    uint8_t g1 = rom.ReadU8(pos + 4);
    uint8_t z1 = rom.ReadU8(pos + 5);
    uint8_t g2 = rom.ReadU8(pos + 6);
    uint8_t z2 = rom.ReadU8(pos + 7);

    if (z1 != 0 || z2 != 0 || g1 != g2)
        return false;

    // now check if the pointer points to a valid song
    if (!validateSong(rom.ReadAgbPtrToPos(pos)))
        return false;
    return true;
}

bool SongTable::validateSong(size_t songPos)
{
    Rom& rom = Rom::Instance();

    uint8_t nTracks = rom.ReadU8(songPos + 0);
    uint8_t nBlocks = rom.ReadU8(songPos + 1);  // these could be anything
    uint8_t prio = rom.ReadU8(songPos + 2);
    uint8_t rev = rom.ReadU8(songPos + 3);

    if ((nTracks | nBlocks | prio | rev) == 0)
        return true;

    // verify voicegroup pointer
    uint32_t voicePtr = rom.ReadU32(songPos + 4);
    if (!rom.ValidPointer(voicePtr))
        return false;

    // verify track pointers
    for (uint32_t i = 0; i < nTracks; i++) {
        uint32_t trackPtr = rom.ReadU32(songPos + 8 + (i * 4));
        if (!rom.ValidPointer(trackPtr))
            return false;
    }

    return true;
}

size_t SongTable::determineNumSongs() const
{
    size_t pos = songTablePos;
    size_t count = 0;
    while (true) {
        if (!validateTableEntry(pos))
            break;
        count++;
        pos += 8;
    }
    return count;
}
