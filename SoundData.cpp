#include <cstring>
#include <algorithm>

#include "AgbTypes.h"
#include "SoundData.h"
#include "MyException.h"
#include "Debug.h"

using namespace agbplay;
using namespace std;

/*
 * public
 * SongTable
 */

SongTable::SongTable(Rom& rrom, long songTable) : rom(rrom) 
{
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

long SongTable::GetPosOfSong(uint16_t uid) {
    rom.Seek(songTable + uid * 8);
    return rom.ReadAGBPtrToPos();
}

unsigned short SongTable::GetNumSongs() {
    return numSongs;
}

/*
 * private
 */

long SongTable::locateSongTable() 
{
    for (long i = 0x200; i < (long)rom.Size(); i += 4) {
        bool validEntries = true;
        long location = i;
        for (long j = 0; j < MIN_SONG_NUM; j++) {
            if (!validateTableEntry(i + j * 8)) {
                validEntries = false;
                break;
            }
        }
        if (validEntries)
            return location;
    }
    throw MyException("Unable to find songtable");
}

bool SongTable::validateTableEntry(long pos) 
{
    rom.Seek(pos);
    agbptr_t songPtr = rom.ReadUInt32();

    // check if the pointer is actually valid
    long debug_pos = 0xFD484;
    if (pos == debug_pos) {
        __print_debug("Checking...");
    }
    if (!rom.ValidPointer(songPtr))
        return false;

    if (pos == debug_pos) {
        __print_debug("Passed pointer test");
    }
    // check if the song groups are set appropriately
    rom.Seek(pos + 4);
    
    uint8_t g1 = rom.ReadUInt8();
    uint8_t z1 = rom.ReadUInt8();
    uint8_t g2 = rom.ReadUInt8();
    uint8_t z2 = rom.ReadUInt8();

    if (z1 != 0 || z2 != 0 || g1 != g2)
        return false;

    if (pos == debug_pos) {
        __print_debug("Passed song group and zero check");
    }
    // now check if the pointer points to a valid song
    if (!validateSong(songPtr))
        return false;
    if (pos == debug_pos)
        __print_debug("Passed actual song test");
    return true;
}

bool SongTable::validateSong(agbptr_t ptr) 
{
    rom.SeekAGBPtr(ptr);
    uint8_t nTracks = rom.ReadUInt8();
    uint8_t nBlocks = rom.ReadUInt8(); // these could be anything
    uint8_t prio = rom.ReadUInt8();
    uint8_t rev = rom.ReadUInt8();

    if ((nTracks | nBlocks | prio | rev) == 0)
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

unsigned short SongTable::determineNumSongs() 
{
    long pos = songTable;
    unsigned short count = 0;
    while (true) {
        if (!validateTableEntry(pos))
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

Sequence::Sequence(long songHeader, uint8_t trackLimit, Rom& rom) : rom(rom)
{
    // read song header
    this->songHeader = songHeader;
    rom.Seek(songHeader);
    uint8_t nTracks = min(rom.ReadUInt8(), trackLimit);
    blocks = rom.ReadUInt8();
    prio = rom.ReadUInt8();
    reverb = rom.ReadUInt8();

    // voicegroup
    soundBank = rom.AGBPtrToPos(rom.ReadUInt32());

    // read track pointer
    tracks.clear();
    for (uint8_t i = 0; i < nTracks; i++) 
    {
        rom.Seek(songHeader + 8 + 4 * i);
        tracks.push_back(Track(rom.ReadAGBPtrToPos()));
    }
    dcont = DisplayContainer(nTracks);

    // reset runtime variables
    bpmStack = 0;
    bpm = 120;
}

Sequence::~Sequence() 
{

}

DisplayContainer& Sequence::GetUpdatedDisp()
{
    for (uint32_t i = 0; i < tracks.size(); i++) 
    {
        dcont.data[i].trackPtr = (uint32_t) tracks[i].pos;
        dcont.data[i].isCalling = (tracks[i].retStackPos == 0) ? true : false;
        dcont.data[i].isMuted = tracks[i].muted;
        dcont.data[i].vol = tracks[i].vol;
        dcont.data[i].mod = tracks[i].mod;
        dcont.data[i].prog = tracks[i].prog;
        dcont.data[i].pan = tracks[i].pan;
        dcont.data[i].pitch = int16_t(tracks[i].bendr * tracks[i].bend + tracks[i].tune * 2);
        dcont.data[i].envL = 0; // FIXME do proper volume scale updating
        dcont.data[i].envR = 0;
        dcont.data[i].delay = uint8_t((tracks[i].delay < 0) ? 0 : tracks[i].delay);
        // TODO do active notes updates
    }
    return dcont;
}

Rom& Sequence::getRom()
{
    return rom;
}

long Sequence::getSndBnk()
{
    return soundBank;
}

/*
 * public
 * Track
 */

Sequence::Track::Track(long pos) 
{
    this->pos = pos;
    retStack[0] = retStack[1] = retStack[2] = patBegin = 0;
    prog = PROG_UNDEFINED;
    vol = 100;
    delay = mod = reptCount = retStackPos = 0;
    pan = bend = tune = keyShift = 0;
    muted = false;
    isRunning = true;
}

Sequence::Track::~Track() 
{
}

int16_t Sequence::Track::GetPitch()
{
    // TODO implement pitch MOD handling
    return bend * bendr + tune * 2;
}

/*
 * public
 * SoundData
 */

SoundData::SoundData(Rom& rrom) 
{
    sTable = new SongTable(rrom, UNKNOWN_TABLE);
}

SoundData::~SoundData() 
{
    delete sTable;
}
