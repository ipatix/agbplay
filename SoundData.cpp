#include <cstring>
#include <cassert>
#include <algorithm>

#include "AgbTypes.h"
#include "SoundData.h"
#include "MyException.h"
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
    rom.Seek(bankPos + instrNum * 12);
    auto instr = (Instrument *)rom.GetPtr();

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
        rom.SeekAGBPtr(instr->field_8.instrMap + midiKey);
        uint8_t mappedInstr = rom.ReadUInt8();
        rom.SeekAGBPtr(instr->field_4.subTable + mappedInstr * 12);
        auto subInstr = (Instrument *)rom.GetPtr();
        return lookup(subInstr->type);
    } else if (instr->type == 0x80) {
        rom.SeekAGBPtr(instr->field_4.subTable + midiKey * 12);
        auto subInstr = (Instrument *)rom.GetPtr();
        return lookup(subInstr->type);
    } else {
        return lookup(instr->type);
    }
}

uint8_t SoundBank::GetMidiKey(uint8_t instrNum, uint8_t midiKey)
{
    rom.Seek(bankPos + instrNum * 12);
    auto instr = (Instrument *)rom.GetPtr();
    if (instr->type == 0x80) {
        rom.Seek(bankPos + midiKey * 12);
        auto subInstr = (Instrument *)rom.GetPtr();
        return subInstr->midiKey;
    } else {
        return midiKey;
    }
    return 0;
}

uint8_t SoundBank::GetPan(uint8_t instrNum, uint8_t midiKey)
{
    rom.Seek(bankPos + instrNum * 12);
    auto instr = (Instrument *)rom.GetPtr();
    if (instr->type == 0x40) {
        rom.SeekAGBPtr(instr->field_8.instrMap + midiKey);
        uint8_t mappedInstr = rom.ReadUInt8();
        rom.SeekAGBPtr(instr->field_4.subTable + mappedInstr * 12);
        auto subInstr = (Instrument *)rom.GetPtr();
        assert(subInstr->type == 0x0 || subInstr->type == 0x8);
        return subInstr->field_3.pan;
    } else if (instr->type == 0x80) {
        rom.SeekAGBPtr(instr->field_4.subTable + midiKey * 12);
        auto subInstr = (Instrument *)rom.GetPtr();
        assert(subInstr->type == 0x0 || subInstr->type == 0x8);
        return subInstr->field_3.pan;
    } else {
        assert(instr->type == 0x0 || instr->type == 0x8);
        return instr->field_3.pan;
    }
}

uint8_t SoundBank::GetSweep(uint8_t instrNum, uint8_t midiKey)
{
    rom.Seek(bankPos + instrNum * 12);
    auto instr = (Instrument *)rom.GetPtr();
    if (instr->type == 0x40) {
        rom.SeekAGBPtr(instr->field_8.instrMap + midiKey);
        uint8_t mappedInstr = rom.ReadUInt8();
        rom.SeekAGBPtr(instr->field_4.subTable + mappedInstr * 12);
        auto subInstr = (Instrument *)rom.GetPtr();
        assert(subInstr->type == 0x1 || subInstr->type == 0x9);
        return subInstr->field_3.sweep;
    } else if (instr->type == 0x80) {
        rom.SeekAGBPtr(instr->field_4.subTable + midiKey * 12);
        auto subInstr = (Instrument *)rom.GetPtr();
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
    rom.Seek(bankPos + instrNum * 12);
    auto instr = (Instrument *)rom.GetPtr();
    if (instr->type == 0x40) {
        rom.SeekAGBPtr(instr->field_8.instrMap + midiKey);
        uint8_t mappedInstr = rom.ReadUInt8();
        rom.SeekAGBPtr(instr->field_4.subTable + mappedInstr * 12);
        auto subInstr = (Instrument *)rom.GetPtr();
        if (subInstr->type == 0x1 || subInstr->type == 0x9 || subInstr->type == 0x2 || subInstr->type == 0xA) {
            switch (subInstr->field_4.dutyCycle) {
                case 0: def.wd = WaveDuty::D12; break;
                case 1: def.wd = WaveDuty::D25; break;
                case 2: def.wd = WaveDuty::D50; break;
                case 3: def.wd = WaveDuty::D75; break;
                default:
                    throw MyException(FormatString("Invalid Square Wave duty cycle at 0x%7X", rom.GetPos()));
            }
        } else if (subInstr->type == 0x3 || subInstr->type == 0xB) {
            rom.SeekAGBPtr(subInstr->field_4.wavePtr);
            def.wavePtr = (uint8_t *)rom.GetPtr();
        } else if (subInstr->type == 0x4 || subInstr->type == 0xC) {
            switch (subInstr->field_4.dutyCycle) {
                case 0: def.np = NoisePatt::FINE; break;
                case 1: def.np = NoisePatt::ROUGH; break;
                default: 
                    throw MyException(FormatString("Invalid Noise Pattern at 0x%7X", rom.GetPos()));
            }
        } else {
            assert(false);
        }
    } else if (instr->type == 0x80) {
        rom.SeekAGBPtr(instr->field_4.subTable + midiKey * 12);
        auto subInstr = (Instrument *)rom.GetPtr();
        if (subInstr->type == 0x1 || subInstr->type == 0x9 || subInstr->type == 0x2 || subInstr->type == 0xA) {
            switch (subInstr->field_4.dutyCycle) {
                case 0: def.wd = WaveDuty::D12; break;
                case 1: def.wd = WaveDuty::D25; break;
                case 2: def.wd = WaveDuty::D50; break;
                case 3: def.wd = WaveDuty::D75; break;
                default:
                    throw MyException(FormatString("Invalid Square Wave duty cycle at 0x%7X", rom.GetPos()));
            }
        } else if (subInstr->type == 0x3 || subInstr->type == 0xB) {
            rom.SeekAGBPtr(subInstr->field_4.wavePtr);
            def.wavePtr = (uint8_t *)rom.GetPtr();
        } else if (subInstr->type == 0x4 || subInstr->type == 0xC) {
            switch (subInstr->field_4.dutyCycle) {
                case 0: def.np = NoisePatt::FINE; break;
                case 1: def.np = NoisePatt::ROUGH; break;
                default: 
                    throw MyException(FormatString("Invalid Noise Pattern at 0x%7X", rom.GetPos()));
            }
        } else {
            assert(false);
        }
    } else {
        if (instr->type == 0x1 || instr->type == 0x9 || instr->type == 0x2 || instr->type == 0xA) {
            switch (instr->field_4.dutyCycle) {
                case 0: def.wd = WaveDuty::D12; break;
                case 1: def.wd = WaveDuty::D25; break;
                case 2: def.wd = WaveDuty::D50; break;
                case 3: def.wd = WaveDuty::D75; break;
                default:
                    throw MyException(FormatString("Invalid Square Wave duty cycle at 0x%7X", rom.GetPos()));
            }
        } else if (instr->type == 0x3 || instr->type == 0xB) {
            rom.SeekAGBPtr(instr->field_4.wavePtr);
            def.wavePtr = (uint8_t *)rom.GetPtr();
        } else if (instr->type == 0x4 || instr->type == 0xC) {
            switch (instr->field_4.dutyCycle) {
                case 0: def.np = NoisePatt::FINE; break;
                case 1: def.np = NoisePatt::ROUGH; break;
                default: 
                    throw MyException(FormatString("Invalid Noise Pattern at 0x%7X", rom.GetPos()));
            }
        } else {
            assert(false);
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
    if (instr->type == 0x40) {
        rom.SeekAGBPtr(instr->field_8.instrMap + midiKey);
        uint8_t mappedInstr = rom.ReadUInt8();
        rom.SeekAGBPtr(instr->field_4.subTable + mappedInstr * 12);
        auto subInstr = (Instrument *)rom.GetPtr();
        assert(subInstr->type == 0x0 || subInstr->type == 0x8);
        rom.SeekAGBPtr(subInstr->field_4.samplePtr);
    } else if (instr->type == 0x80) {
        rom.SeekAGBPtr(instr->field_4.subTable + midiKey * 12);
        auto subInstr = (Instrument *)rom.GetPtr();
        assert(subInstr->type == 0x0 || subInstr->type == 0x8);
        rom.SeekAGBPtr(subInstr->field_4.samplePtr);
    } else {
        assert(instr->type == 0x0 || instr->type == 0x8);
        rom.SeekAGBPtr(instr->field_4.samplePtr);
    }
    uint32_t tmp = rom.ReadUInt32();
    if (tmp == 0x40000000)
        loopEnabled = true;
    else if (tmp == 0x0)
        loopEnabled = false;
    else
        throw MyException(FormatString("Invalid sample mode 0x%8X at 0x%7X", tmp, rom.GetPos()));
    midCfreq = float(rom.ReadUInt32()) / 1024.0f;
    loopPos = rom.ReadUInt32();
    endPos = rom.ReadUInt32();
    samplePtr = (int8_t *)rom.GetPtr();
    return SampleInfo(samplePtr, midCfreq, loopEnabled, loopPos, endPos);
}

ADSR SoundBank::GetADSR(uint8_t instrNum, uint8_t midiKey)
{
    rom.Seek(bankPos + instrNum * 12);
    auto instr = (Instrument *)rom.GetPtr();
    if (instr->type == 0x40) {
        rom.SeekAGBPtr(instr->field_8.instrMap + midiKey);
        uint8_t mappedInstr = rom.ReadUInt8();
        rom.SeekAGBPtr(instr->field_4.subTable + mappedInstr * 12);
        auto subInstr = (Instrument *)rom.GetPtr();
        assert(subInstr->type == 0x0 || subInstr->type == 0x1 || 
                subInstr->type == 0x2 || subInstr->type == 0x3 ||
                subInstr->type == 0x4 || subInstr->type == 0x8 ||
                subInstr->type == 0x9 || subInstr->type == 0xA ||
                subInstr->type == 0xB || subInstr->type == 0xC);
        return subInstr->field_8.env;
    } else if (instr->type == 0x80) {
        rom.SeekAGBPtr(instr->field_4.subTable + midiKey * 12);
        auto subInstr = (Instrument *)rom.GetPtr();
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
        dcont.data[i].isCalling = tracks[i].isCalling;
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
    patBegin = returnPos = 0;
    modt = MODT::PITCH;
    lastEvent = LEvent::NONE;
    lastNoteKey = 60;
    lastNoteVel = 127;
    lastNoteLen = 96;
    prog = PROG_UNDEFINED;
    vol = 100;
    delay = mod = reptCount = bendr = lfos = lfodl = 
        lfodlCount = lfoPhase = echoVol = echoLen = 0;
    pan = bend = tune = keyShift = 0;
    muted = isCalling = false;
    isRunning = true;
}

Sequence::Track::~Track() 
{
}

const vector<int16_t> Sequence::sineLut = {
    0, 19, 38, 56, 75, 94, 113, 131, 150, 168, 186, 205, 223, 241, 258, 276, 
    294, 311, 328, 345, 362, 378, 394, 410, 426, 442, 457, 472, 487, 501, 515, 529, 
    542, 555, 568, 581, 593, 605, 616, 627, 638, 648, 658, 667, 676, 685, 693, 701, 
    709, 716, 722, 728, 734, 739, 744, 748, 752, 756, 759, 761, 763, 765, 766, 767, 
    767, 767, 766, 765, 763, 761, 759, 756, 752, 748, 744, 739, 734, 728, 722, 716, 
    709, 701, 693, 685, 676, 667, 658, 648, 638, 627, 616, 605, 593, 581, 568, 555, 
    542, 529, 515, 501, 487, 472, 457, 442, 426, 410, 394, 378, 362, 345, 328, 311, 
    294, 276, 258, 241, 223, 205, 186, 168, 150, 131, 113, 94, 75, 56, 38, 19, 
    0, -19, -38, -56, -75, -94, -113, -131, -150, -168, -186, -205, -223, -241, -258, -276, 
    -294, -311, -328, -345, -362, -378, -394, -410, -426, -442, -457, -472, -487, -501, -515, -529, 
    -542, -555, -568, -581, -593, -605, -616, -627, -638, -648, -658, -667, -676, -685, -693, -701, 
    -709, -716, -722, -728, -734, -739, -744, -748, -752, -756, -759, -761, -763, -765, -766, -767, 
    -767, -767, -766, -765, -763, -761, -759, -756, -752, -748, -744, -739, -734, -728, -722, -716, 
    -709, -701, -693, -685, -676, -667, -658, -648, -638, -627, -616, -605, -593, -581, -568, -555, 
    -542, -529, -515, -501, -487, -472, -457, -442, -426, -410, -394, -378, -362, -345, -328, -311, 
    -294, -276, -258, -241, -223, -205, -186, -168, -150, -131, -113, -94, -75, -56, -38, -19 
};

int16_t Sequence::Track::GetPitch()
{
    return int16_t(bend * bendr + tune * 2 + sineLut[lfoPhase] / 128);
}

uint8_t Sequence::Track::GetLeftVol()
{
    return uint8_t(vol * (-pan + 64) / 64); // maybe devide by 128 so nothing overflows
}

uint8_t Sequence::Track::GetRightVol()
{
    return uint8_t(vol * (pan + 64) / 64);
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
