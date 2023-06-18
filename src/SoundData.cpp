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
 * public SoundBank
 */

void SoundBank::Init(size_t bankPos)
{
    this->bankPos = bankPos;
}

InstrType SoundBank::GetInstrType(uint8_t instrNum, uint8_t midiKey)
{
    Rom& rom = Rom::Instance();
    size_t pos = instrPos(instrNum, midiKey);

    switch (rom.ReadU8(pos + 0x0)) {
    case 0x0:
        return InstrType::PCM;
    case 0x1:
        return InstrType::SQ1;
    case 0x2:
        return InstrType::SQ2;
    case 0x3:
        return InstrType::WAVE;
    case 0x4:
        return InstrType::NOISE;
    case 0x8:
        return InstrType::PCM_FIXED;
    case 0x9:
        return InstrType::SQ1;
    case 0xA:
        return InstrType::SQ2;
    case 0xB:
        return InstrType::WAVE;
    case 0xC:
        return InstrType::NOISE;
    default:
        return InstrType::INVALID;
    }
}

uint8_t SoundBank::GetMidiKey(uint8_t instrNum, uint8_t midiKey)
{
    Rom& rom = Rom::Instance();

    if (rom.ReadU8(bankPos + instrNum * 12 + 0) == 0x80) {
        size_t subBankPos = rom.ReadAgbPtrToPos(bankPos + instrNum * 12 + 0x4);
        return rom.ReadU8(subBankPos + midiKey * 12 + 1);
    } else {
        return midiKey;
    }
}

int8_t SoundBank::GetPan(uint8_t instrNum, uint8_t midiKey)
{
    size_t pos = instrPos(instrNum, midiKey);

    // not strictly correct, pan should only be set for drum table instruments
    InstrType t = GetInstrType(instrNum, midiKey);
    if (t != InstrType::PCM && t != InstrType::PCM_FIXED)
        return 0;

    uint8_t pan = Rom::Instance().ReadU8(pos + 3);
    if (pan & 0x80)
        return static_cast<int8_t>(pan - 0xC0);
    else
        return 0;
}

uint8_t SoundBank::GetSweep(uint8_t instrNum, uint8_t midiKey)
{
    size_t pos = instrPos(instrNum, midiKey);
    InstrType t = GetInstrType(instrNum, midiKey);
    if (t != InstrType::SQ1)
        throw Xcept("SoundBank Error: Invalid use of sweep at non SQ1 instrument: [%08X]", pos);

    return Rom::Instance().ReadU8(pos + 3);
}

CGBDef SoundBank::GetCGBDef(uint8_t instrNum, uint8_t midiKey)
{
    Rom& rom = Rom::Instance();
    CGBDef def;

    size_t pos = instrPos(instrNum, midiKey);
    InstrType t = GetInstrType(instrNum, midiKey);

    if (t == InstrType::SQ1 || t == InstrType::SQ2) {
        uint32_t dutyCycle = rom.ReadU32(pos + 4);
        switch (dutyCycle) {
        case 0: def.wd = WaveDuty::D12; break;
        case 1: def.wd = WaveDuty::D25; break;
        case 2: def.wd = WaveDuty::D50; break;
        case 3: def.wd = WaveDuty::D75; break;
        default:
            throw Xcept("SoundBank Error: Invalid square wave duty cycle at [%08X+4]=%08X", pos, dutyCycle);
        }
    } else if (t == InstrType::WAVE) {
        def.wavePtr = static_cast<const uint8_t *>(rom.GetPtr(rom.ReadAgbPtrToPos(pos + 4)));
    } else if (t == InstrType::NOISE) {
        uint32_t noisePatt = rom.ReadU32(pos + 4);
        switch (noisePatt) {
        case 0: def.np = NoisePatt::FINE; break;
        case 1: def.np = NoisePatt::ROUGH; break;
        default:
            throw Xcept("SoundBank Error: Invalid noise pattern at [%08X+4]=%08X", pos, noisePatt);
        }
    } else {
        throw Xcept("SoundBank Error: Cannot get CGB definition of instrument: [%08X]", pos);
    }

    return def;
}

SampleInfo SoundBank::GetSampInfo(uint8_t instrNum, uint8_t midiKey)
{
    Rom& rom = Rom::Instance();

    size_t pos = instrPos(instrNum, midiKey);
    InstrType t = GetInstrType(instrNum, midiKey);
    if (t != InstrType::PCM && t != InstrType::PCM_FIXED)
        throw Xcept("SoundBank Error: Cannot get sample info of non PCM instrument: [%08X]", pos);

    size_t samplePos = rom.ReadAgbPtrToPos(pos + 4);

    bool loopEnabled = rom.ReadU8(samplePos + 3) & 0xC0;
    if (rom.ReadU8(samplePos) != 0)
        throw Xcept("Sample Error: Unknown/unsupported sample mode: [%08X]=%02X, instrument: [%08X]",
                samplePos, rom.ReadU8(samplePos), pos);

    float midCfreq = static_cast<float>(rom.ReadU32(samplePos + 4)) / 1024.0f;
    uint32_t loopPos = rom.ReadU32(samplePos + 8);
    uint32_t endPos = rom.ReadU32(samplePos + 12);
    const int8_t *samplePtr = static_cast<const int8_t *>(rom.GetPtr(samplePos + 16));
    return SampleInfo(samplePtr, midCfreq, loopEnabled, loopPos, endPos);
}

ADSR SoundBank::GetADSR(uint8_t instrNum, uint8_t midiKey)
{
    Rom& rom = Rom::Instance();

    size_t pos = instrPos(instrNum, midiKey);
    InstrType t = GetInstrType(instrNum, midiKey);
    if (t == InstrType::INVALID)
        throw Xcept("SoundBank Error: Cannot get ADSR for unknown instrument type: [%08X]", pos);

    ADSR adsr;
    adsr.att = rom.ReadU8(pos + 8);
    adsr.dec = rom.ReadU8(pos + 9);
    adsr.sus = rom.ReadU8(pos + 10);
    adsr.rel = rom.ReadU8(pos + 11);
    return adsr;
}

size_t SoundBank::instrPos(uint8_t instrNum, uint8_t midiKey) {
    Rom& rom = Rom::Instance();
    uint8_t type = rom.ReadU8(bankPos + instrNum * 12 + 0x0);

    if (type == 0x80) {
        size_t subBankPos = rom.ReadAgbPtrToPos(bankPos + instrNum * 12 + 0x4);
        return subBankPos + midiKey * 12;
    } else if (type == 0x40) {
        size_t subBankPos = rom.ReadAgbPtrToPos(bankPos + instrNum * 12 + 0x4);
        size_t keyMapPos = rom.ReadAgbPtrToPos(bankPos + instrNum * 12 + 0x8);
        return subBankPos + rom.ReadU8(keyMapPos + midiKey) * 12;
    } else {
        return bankPos + instrNum * 12;
    }
}

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

size_t Sequence::GetSoundBankPos()
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

uint8_t Track::GetVol()
{
    int v = vol << 1;
    if (modt == MODT::VOL)
        v = (v * (lfoValue + 128)) >> 7;
    v >>= 1;
    return static_cast<uint8_t>(v);
}

int8_t Track::GetPan()
{
    int p = pan << 1;
    if (modt == MODT::PAN)
        p += lfoValue;
    p /= 2;

    return static_cast<int8_t>(std::clamp(p, -64, 63));
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

SongTable::SongTable(size_t songTablePos)
    : songTablePos(songTablePos)
{
    if (this->songTablePos == UNKNOWN_TABLE) {
        this->songTablePos = locateSongTable();
    }
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

size_t SongTable::locateSongTable() 
{
    Rom& rom = Rom::Instance();

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
                if (value == location)
                    return location;
            }
            i += j * 8;
        }
    }

    for (size_t i = 0x200; i < rom.Size(); i += 4) {
        u_int32_t check = rom.ReadU32(i);
        if (check == 0x10002) {
            return i + 0xc;
        }
    }

    throw Xcept("Unable to find songtable");
}

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

size_t SongTable::determineNumSongs() 
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
