#include <cstring>
#include <iostream>
#include <cassert>
#include <algorithm>

#include "AgbTypes.h"
#include "SoundData.h"
#include "Xcept.h"
#include "Debug.h"
#include "Util.h"

using namespace agbplay;
using namespace std;

/*
 * public SoundBank
 */

SoundBank::SoundBank(Rom& rom, long bankPos) : rom(rom)
{
    this->bankPos = bankPos;
}

SoundBank::~SoundBank()
{
}

InstrType SoundBank::GetInstrType(uint8_t instrNum, uint8_t midiKey)
{
    auto instr = (Instrument *)&rom[bankPos + instrNum * 12];
    auto lookup = [](uint8_t key) {
        switch (key) {
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
    };

    if (instr->type == 0x40) {
        uint8_t mappedInstr = rom[rom.AGBPtrToPos(instr->field_8.instrMap) + midiKey];
        auto subInstr = (Instrument *)&rom[rom.AGBPtrToPos(instr->field_4.subTable) + mappedInstr * 12];
        return lookup(subInstr->type);
    } else if (instr->type == 0x80) {
        auto subInstr = (Instrument *)&rom[rom.AGBPtrToPos(instr->field_4.subTable) + midiKey * 12];
        return lookup(subInstr->type);
    } else {
        return lookup(instr->type);
    }
}

uint8_t SoundBank::GetMidiKey(uint8_t instrNum, uint8_t midiKey)
{
    auto instr = (Instrument *)&rom[bankPos + instrNum * 12];
    if (instr->type == 0x80) {
        auto subInstr = (Instrument *)&rom[rom.AGBPtrToPos(instr->field_4.subTable) + midiKey * 12];
        return subInstr->midiKey;
    } else {
        return midiKey;
    }
}

uint8_t SoundBank::GetPan(uint8_t instrNum, uint8_t midiKey)
{
    auto instr = (Instrument *)&rom[bankPos + instrNum * 12];
    if (instr->type == 0x40) {
        uint8_t mappedInstr = rom[rom.AGBPtrToPos(instr->field_8.instrMap) + midiKey];
        auto subInstr = (Instrument *)&rom[rom.AGBPtrToPos(instr->field_4.subTable) + mappedInstr * 12];
        assert(subInstr->type == 0x0 || subInstr->type == 0x8);
        return subInstr->field_3.pan;
    } else if (instr->type == 0x80) {
        auto subInstr = (Instrument *)&rom[rom.AGBPtrToPos(instr->field_4.subTable) + midiKey * 12];
        assert(subInstr->type == 0x0 || subInstr->type == 0x8);
        return subInstr->field_3.pan;
    } else {
        assert(instr->type == 0x0 || instr->type == 0x8);
        return instr->field_3.pan;
    }
}

uint8_t SoundBank::GetSweep(uint8_t instrNum, uint8_t midiKey)
{
    auto instr = (Instrument *)&rom[bankPos + instrNum * 12];
    if (instr->type == 0x40) {
        uint8_t mappedInstr = rom[rom.AGBPtrToPos(instr->field_8.instrMap) + midiKey];
        auto subInstr = (Instrument *)&rom[rom.AGBPtrToPos(instr->field_4.subTable) + mappedInstr * 12];
        assert(subInstr->type == 0x1 || subInstr->type == 0x9);
        return subInstr->field_3.sweep;
    } else if (instr->type == 0x80) {
        auto subInstr = (Instrument *)&rom[rom.AGBPtrToPos(instr->field_4.subTable) + midiKey * 12];
        assert(subInstr->type == 0x1 || subInstr->type == 0x9);
        return subInstr->field_3.sweep;
    } else {
        assert(instr->type == 0x1 || instr->type == 0x9);
        return instr->field_3.sweep;
    }
}

CGBDef SoundBank::GetCGBDef(uint8_t instrNum, uint8_t midiKey)
{
    CGBDef def;
    auto instr = (Instrument *)&rom[bankPos + instrNum * 12];
    if (instr->type == 0x40) {
        uint8_t mappedInstr = rom[rom.AGBPtrToPos(instr->field_8.instrMap) + midiKey];
        auto subInstr = (Instrument *)&rom[rom.AGBPtrToPos(instr->field_4.subTable) + mappedInstr * 12];
        if (subInstr->type == 0x1 || subInstr->type == 0x9 || subInstr->type == 0x2 || subInstr->type == 0xA) {
            switch (subInstr->field_4.dutyCycle) {
            case 0: def.wd = WaveDuty::D12; break;
            case 1: def.wd = WaveDuty::D25; break;
            case 2: def.wd = WaveDuty::D50; break;
            case 3: def.wd = WaveDuty::D75; break;
            default:
                    throw Xcept("Invalid Square Wave duty cycle at 0x%07X", rom.GetPos());
            }
        } else if (subInstr->type == 0x3 || subInstr->type == 0xB) {
            def.wavePtr = &rom[rom.AGBPtrToPos(subInstr->field_4.wavePtr)];
        } else if (subInstr->type == 0x4 || subInstr->type == 0xC) {
            switch (subInstr->field_4.dutyCycle) {
            case 0: def.np = NoisePatt::FINE; break;
            case 1: def.np = NoisePatt::ROUGH; break;
            default:
                    throw Xcept("Invalid Noise Pattern at 0x%07X", rom.GetPos());
            }
        } else {
            throw Xcept("Illegal Instrument at 0x%07X", rom.GetPos());
        }
    } else if (instr->type == 0x80) {
        auto subInstr = (Instrument *)&rom[rom.AGBPtrToPos(instr->field_4.subTable) + midiKey * 12];
        if (subInstr->type == 0x1 || subInstr->type == 0x9 || subInstr->type == 0x2 || subInstr->type == 0xA) {
            switch (subInstr->field_4.dutyCycle) {
            case 0: def.wd = WaveDuty::D12; break;
            case 1: def.wd = WaveDuty::D25; break;
            case 2: def.wd = WaveDuty::D50; break;
            case 3: def.wd = WaveDuty::D75; break;
            default:
                    throw Xcept("Invalid Square Wave duty cycle at 0x%07X", rom.GetPos());
            }
        } else if (subInstr->type == 0x3 || subInstr->type == 0xB) {
            def.wavePtr = &rom[rom.AGBPtrToPos(subInstr->field_4.wavePtr)];
        } else if (subInstr->type == 0x4 || subInstr->type == 0xC) {
            switch (subInstr->field_4.dutyCycle) {
            case 0: def.np = NoisePatt::FINE; break;
            case 1: def.np = NoisePatt::ROUGH; break;
            default:
                    throw Xcept("Invalid Noise Pattern at 0x%07X", rom.GetPos());
            }
        } else {
            throw Xcept("Illegal Instrument at 0x%07X", rom.GetPos());
        }
    } else {
        if (instr->type == 0x1 || instr->type == 0x9 || instr->type == 0x2 || instr->type == 0xA) {
            switch (instr->field_4.dutyCycle) {
            case 0: def.wd = WaveDuty::D12; break;
            case 1: def.wd = WaveDuty::D25; break;
            case 2: def.wd = WaveDuty::D50; break;
            case 3: def.wd = WaveDuty::D75; break;
            default:
                    throw Xcept("Invalid Square Wave duty cycle at 0x%07X", rom.GetPos());
            }
        } else if (instr->type == 0x3 || instr->type == 0xB) {
            def.wavePtr = &rom[rom.AGBPtrToPos(instr->field_4.wavePtr)];
        } else if (instr->type == 0x4 || instr->type == 0xC) {
            switch (instr->field_4.dutyCycle) {
            case 0: def.np = NoisePatt::FINE; break;
            case 1: def.np = NoisePatt::ROUGH; break;
            default:
                    throw Xcept("Invalid Noise Pattern at 0x%07X", rom.GetPos());
            }
        } else {
            throw Xcept("Illegal Instrument at 0x%07X", rom.GetPos());
        }
    }
    return def;
}

SampleInfo SoundBank::GetSampInfo(uint8_t instrNum, uint8_t midiKey)
{
    int8_t *samplePtr;
    float midCfreq;
    uint32_t loopPos;
    uint32_t endPos;
    bool loopEnabled;

    rom.Seek(bankPos + instrNum * 12);
    auto instr = (Instrument *)rom.GetPtr();
    long sampHeaderPos;
    if (instr->type == 0x40) {
        uint8_t mappedInstr = rom[rom.AGBPtrToPos(instr->field_8.instrMap) + midiKey];
        auto subInstr = (Instrument *)&rom[rom.AGBPtrToPos(instr->field_4.subTable) + mappedInstr * 12];
        assert(subInstr->type == 0x0 || subInstr->type == 0x8);
        sampHeaderPos = rom.AGBPtrToPos(subInstr->field_4.samplePtr);
    } else if (instr->type == 0x80) {
        auto subInstr = (Instrument *)&rom[rom.AGBPtrToPos(instr->field_4.subTable) + midiKey * 12];
        assert(subInstr->type == 0x0 || subInstr->type == 0x8);
        sampHeaderPos = rom.AGBPtrToPos(subInstr->field_4.samplePtr);
    } else {
        assert(instr->type == 0x0 || instr->type == 0x8);
        sampHeaderPos = rom.AGBPtrToPos(instr->field_4.samplePtr);
    }
    if (*(uint32_t *)&rom[sampHeaderPos + 0x0] == 0x40000000)
        loopEnabled = true;
    else if (rom[sampHeaderPos + 0x0] == 0x0)
        loopEnabled = false;
    else
        throw Xcept("Invalid sample mode 0x%08X at 0x%07X", *(uint32_t *)&rom[sampHeaderPos + 0x0], sampHeaderPos);
    midCfreq = float(*(uint32_t *)&rom[sampHeaderPos + 0x4]) / 1024.0f;
    loopPos = *(uint32_t *)&rom[sampHeaderPos + 0x8];
    endPos = *(uint32_t *)&rom[sampHeaderPos + 0xC];
    samplePtr = (int8_t *)&rom[sampHeaderPos + 0x10];
    return SampleInfo(samplePtr, midCfreq, loopEnabled, loopPos, endPos);
}

ADSR SoundBank::GetADSR(uint8_t instrNum, uint8_t midiKey)
{
    auto instr = (Instrument *)&rom[bankPos + instrNum * 12];
    if (instr->type == 0x40) {
        uint8_t mappedInstr = rom[rom.AGBPtrToPos(instr->field_8.instrMap) + midiKey];
        auto subInstr = (Instrument *)&rom[rom.AGBPtrToPos(instr->field_4.subTable) + mappedInstr * 12];
        assert(subInstr->type == 0x0 || subInstr->type == 0x1 || 
                subInstr->type == 0x2 || subInstr->type == 0x3 ||
                subInstr->type == 0x4 || subInstr->type == 0x8 ||
                subInstr->type == 0x9 || subInstr->type == 0xA ||
                subInstr->type == 0xB || subInstr->type == 0xC);
        return subInstr->field_8.env;
    } else if (instr->type == 0x80) {
        auto subInstr = (Instrument *)&rom[rom.AGBPtrToPos(instr->field_4.subTable) + midiKey * 12];
        assert(subInstr->type == 0x0 || subInstr->type == 0x1 || 
                subInstr->type == 0x2 || subInstr->type == 0x3 ||
                subInstr->type == 0x4 || subInstr->type == 0x8 ||
                subInstr->type == 0x9 || subInstr->type == 0xA ||
                subInstr->type == 0xB || subInstr->type == 0xC);
        return subInstr->field_8.env;
    } else {
        assert(instr->type == 0x0 || instr->type == 0x1 || 
                instr->type == 0x2 || instr->type == 0x3 ||
                instr->type == 0x4 || instr->type == 0x8 ||
                instr->type == 0x9 || instr->type == 0xA ||
                instr->type == 0xB || instr->type == 0xC);
        return instr->field_8.env;
    }
}

/*
 * public Sequence
 */

Sequence::Sequence(long songHeader, uint8_t trackLimit, Rom& rom) : rom(rom)
{
    // read song header
    this->songHeader = songHeader;
    rom.Seek(songHeader);
    uint8_t nTracks = min<uint8_t>(rom.ReadUInt8(), trackLimit);
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

    // reset runtime variables
    bpmStack = 0;
    bpm = 150;
}

Sequence::~Sequence() 
{
}

Rom& Sequence::GetRom()
{
    return rom;
}

long Sequence::GetSndBnk()
{
    return soundBank;
}

uint8_t Sequence::GetReverb()
{
    return reverb;
}

/*
 * public
 * Track
 */

Sequence::Track::Track(long pos) 
{
    // TODO corrently init all values
    this->pos = pos;
    activeNotes.reset();
    patBegin = returnPos = 0;
    modt = MODT::PITCH;
    lastEvent = LEvent::NONE;
    lastNoteKey = 60;
    lastNoteVel = 127;
    lastNoteLen = 96;
    prog = PROG_UNDEFINED;
    vol = 100;
    lfos = 22;
    delay = mod = reptCount = lfodl = 
        lfodlCount = lfoPhase = echoVol = echoLen = 0;
    bendr = 2;
    pan = bend = tune = keyShift = 0;
    muted = false;
    isRunning = true;
    pitch = 0;
}

Sequence::Track::~Track() 
{
}

const vector<int16_t> Sequence::triLut = {
    0, -12, -24, -36, -48, -60, -72, -84, -96, -108, -120, -132, -144, -156, -168, -180,
    -192, -204, -216, -228, -240, -252, -264, -276, -288, -300, -312, -324, -336, -348, -360, -372,
    -384, -396, -408, -420, -432, -444, -456, -468, -480, -492, -504, -516, -528, -540, -552, -564,
    -576, -588, -600, -612, -624, -636, -648, -660, -672, -684, -696, -708, -720, -732, -744, -756,
    -768, -756, -744, -732, -720, -708, -696, -684, -672, -660, -648, -636, -624, -612, -600, -588,
    -576, -564, -552, -540, -528, -516, -504, -492, -480, -468, -456, -444, -432, -420, -408, -396,
    -384, -372, -360, -348, -336, -324, -312, -300, -288, -276, -264, -252, -240, -228, -216, -204,
    -192, -180, -168, -156, -144, -132, -120, -108, -96, -84, -72, -60, -48, -36, -24, -12,
    0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144, 156, 168, 180,
    192, 204, 216, 228, 240, 252, 264, 276, 288, 300, 312, 324, 336, 348, 360, 372,
    384, 396, 408, 420, 432, 444, 456, 468, 480, 492, 504, 516, 528, 540, 552, 564,
    576, 588, 600, 612, 624, 636, 648, 660, 672, 684, 696, 708, 720, 732, 744, 756,
    768, 756, 744, 732, 720, 708, 696, 684, 672, 660, 648, 636, 624, 612, 600, 588,
    576, 564, 552, 540, 528, 516, 504, 492, 480, 468, 456, 444, 432, 420, 408, 396,
    384, 372, 360, 348, 336, 324, 312, 300, 288, 276, 264, 252, 240, 228, 216, 204,
    192, 180, 168, 156, 144, 132, 120, 108, 96, 84, 72, 60, 48, 36, 24, 12
};

int16_t Sequence::Track::GetPitch()
{
    int m = (modt == MODT::PITCH) ? (triLut[lfoPhase] * mod) >> 8 : 0;
    return int16_t(bend * bendr + tune + m);
}

uint8_t Sequence::Track::GetVol()
{
    int m = (modt == MODT::VOL) ? (triLut[lfoPhase] * mod * 3 * vol) >> 19 : 0;
    return uint8_t(clip(0, vol + m, 127));
}

int8_t Sequence::Track::GetPan()
{
    int m = (modt == MODT::PAN) ? (triLut[lfoPhase] * mod * 3) >> 12 : 0;
    return int8_t(clip(-64, pan + m, 63));
}

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
        long j = 0;
        for (j = 0; j < MIN_SONG_NUM; j++) {
            if (!validateTableEntry(i + j * 8)) {
                i += j * 8;
                validEntries = false;
                break;
            }
        }
        if (validEntries) {
            // before returning, check if reference to song table exists
            rom.Seek(0x200);
            for (long k = 0x200; k < (long)rom.Size() - 3; k += 4) { // -3 due to possible alignment issues
                long value = (long)rom.ReadUInt32() - 0x8000000;
                if (value == location)
                    return location;
            }
            i += j * 8;
        }
    }
    throw Xcept("Unable to find songtable");
}

bool SongTable::validateTableEntry(long pos) 
{
    // check if the pointer is actually valid
    /*long debug_pos = 0x6C5BDC;
      if (pos == debug_pos) {
      _print_debug("Checking...");
      }*/
    rom.Seek(pos);

    agbptr_t songPtr = rom.ReadUInt32();


    if (!rom.ValidPointer(songPtr))
        return false;

    /*
       if (pos == debug_pos) {
       _print_debug("Passed pointer test");
       }*/
    // check if the song groups are set appropriately
    rom.Seek(pos + 4);

    uint8_t g1 = rom.ReadUInt8();
    uint8_t z1 = rom.ReadUInt8();
    uint8_t g2 = rom.ReadUInt8();
    uint8_t z2 = rom.ReadUInt8();

    if (z1 != 0 || z2 != 0 || g1 != g2)
        return false;

    /*if (pos == debug_pos) {
      _print_debug("Passed song group and zero check");
      }*/
    // now check if the pointer points to a valid song
    if (!validateSong(songPtr))
        return false;
    /*if (pos == debug_pos)
      _print_debug("Passed actual song test");*/
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
