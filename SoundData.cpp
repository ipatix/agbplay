#include <cstring>

#include "AgbTypes.h"
#include "SoundData.h"
#include "MyException.h"
#include "Debug.h"

using namespace agbplay;

/*
 * public
 * SongTable
 */

SongTable::SongTable(Rom& rrom, long songTable) : rom(rrom) {
    if (songTable == UNKNOWN_TABLE) {
        this->songTable = locateSongTable();
    } else {
        this->songTable = songTable;
    }
    numSongs = determineNumSongs();
}

SongTable::~SongTable() {
}

long SongTable::GetSongTablePos() {
    return songTable;
}

unsigned short SongTable::GetNumSongs() {
    return numSongs;
}

/*
 * private
 */

long SongTable::locateSongTable() {
    for (long i = 0x200; i < (long)rom.Size(); i += 4) {
        bool validEntries = true;
        long location = i;
        for (long j = 0; j < MIN_SONG_NUM; j++) {
            if (!validateTableEntry(i + j * 8, true)) {
                validEntries = false;
                break;
            }
        }
        if (validEntries)
            return location;
    }
    throw MyException("Unable to find songtable");
}

bool SongTable::validateTableEntry(long pos, bool strongCheck) {
    rom.Seek(pos);
    agbptr_t songPtr = rom.ReadUInt32();

    // check if the pointer is actually valid
    if (!rom.ValidPointer(songPtr))
        return false;

    // check if the song groups are set appropriately
    rom.Seek(pos + 4);
    
    uint8_t g1 = rom.ReadUInt8();
    uint8_t z1 = rom.ReadUInt8();
    uint8_t g2 = rom.ReadUInt8();
    uint8_t z2 = rom.ReadUInt8();

    if (z1 != 0 || z2 != 0 || g1 != g2)
        return false;

    // now check if the pointer points to a valid song
    if (!validateSong(songPtr, strongCheck))
        return false;
    return true;
}

bool SongTable::validateSong(agbptr_t ptr, bool strongCheck) {
    rom.SeekAGBPtr(ptr);
    uint8_t nTracks = rom.ReadUInt8();
    uint8_t nBlocks = rom.ReadUInt8(); // these could be anything
    uint8_t prio = rom.ReadUInt8();
    uint8_t rev = rom.ReadUInt8();

    if (!strongCheck && nTracks | nBlocks | prio | rev == 0)
        return true;

    // verify voicegroup pointer
    agbptr_t voicePtr = rom.ReadUInt32();
    if (!rom.ValidPointer(voicePtr))
        return false;

    // verify track pointers
    for (uint32_t i = 0; i < nTracks; i++) {
        rom.SeekAGBPtr(ptr + 8 + (i * 4));
        agbptr_t trackPtr = rom.ReadUInt32();
        if (!rom.ValidPointer(trackPtr))
            return false;
    }

    return true;
}

unsigned short SongTable::determineNumSongs() {
    long pos = songTable;
    unsigned short count = 0;
    while (true) {
        if (!validateTableEntry(pos, false))
            break;
        count++;
        pos += 8;
    }
    return count;
}

/*
 * public
 * Sequence
 */

SongTable::Sequence::Sequence(long songHeader) {
    // read song header
    rom.Seek(songHeader);
    uint8_t nTracks = rom.ReadUInt8();
    blocks = rom.ReadUInt8();
    prio = rom.ReadUInt8();
    reverb = rom.ReadUInt8();
    // voicegroup
    soundBank = rom.ReadAGBPtrToPos();
    // read track pointer
    track.clear();
    for (uint8_t i = 0; i < nTracks; i++) {
        rom.Seek(songHeader + 8 + 4 * i);
        track.push_back(Track(rom.ReadAGBPtrToPos()));
    }
}

SongTable::Sequence::~Sequence() {

}

/*
 * public
 * Track
 */

SongTable::Sequence::Track::Track(long pos) {
    this->pos = pos;
    retStack[0] = retStack[1] = retStack[2] = patBegin = 0;
    prog = PROG_UNDEFINED;
    delay = vol = mod = bendr = 0;
    pan = bend = tune = keyShift = 0;
    muted = false;
}

SongTable::Sequence::Track::~Track() {

}

/*
 * public
 * SoundData
 */

SoundData::SoundData(Rom& rrom) {   
    sTable = new SongTable(rrom, UNKNOWN_TABLE);
}

SoundData::~SoundData() {
    delete sTable;
}
