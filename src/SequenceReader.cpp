#include <cmath>

#include "SequenceReader.h"
#include "Xcept.h"
#include "Util.h"
#include "Debug.h"
#include "Rom.h"
#include "PlayerContext.h"
#include "ConfigManager.h"

#define SONG_FADE_OUT_TIME 10000
#define SONG_FINISH_TIME 1000
#define NOTE_TIE -1
#define NOTE_ALL 0xFE

/*
 * SequenceReader data
 */

const std::vector<uint32_t> SequenceReader::freqLut = {
    5734, 7884, 10512, 13379,
    15768, 18157, 21024, 26758,
    31536, 36314, 40137, 42048
};

const std::map<uint8_t, int8_t> SequenceReader::delayLut = {
    {0x81,1 }, {0x82,2 }, {0x83,3 }, {0x84,4 }, {0x85,5 }, {0x86,6 }, {0x87,7 }, {0x88,8 },
    {0x89,9 }, {0x8A,10}, {0x8B,11}, {0x8C,12}, {0x8D,13}, {0x8E,14}, {0x8F,15}, {0x90,16},
    {0x91,17}, {0x92,18}, {0x93,19}, {0x94,20}, {0x95,21}, {0x96,22}, {0x97,23}, {0x98,24},
    {0x99,28}, {0x9A,30}, {0x9B,32}, {0x9C,36}, {0x9D,40}, {0x9E,42}, {0x9F,44}, {0xA0,48},
    {0xA1,52}, {0xA2,54}, {0xA3,56}, {0xA4,60}, {0xA5,64}, {0xA6,66}, {0xA7,68}, {0xA8,72},
    {0xA9,76}, {0xAA,78}, {0xAB,80}, {0xAC,84}, {0xAD,88}, {0xAE,90}, {0xAF,92}, {0xB0,96}
};

const std::map<uint8_t, int8_t> SequenceReader::noteLut = {
    {0xD0,1 }, {0xD1,2 }, {0xD2,3 }, {0xD3,4 }, {0xD4,5 }, {0xD5,6 }, {0xD6,7 }, {0xD7,8 },
    {0xD8,9 }, {0xD9,10}, {0xDA,11}, {0xDB,12}, {0xDC,13}, {0xDD,14}, {0xDE,15}, {0xDF,16},
    {0xE0,17}, {0xE1,18}, {0xE2,19}, {0xE3,20}, {0xE4,21}, {0xE5,22}, {0xE6,23}, {0xE7,24},
    {0xE8,28}, {0xE9,30}, {0xEA,32}, {0xEB,36}, {0xEC,40}, {0xED,42}, {0xEE,44}, {0xEF,48},
    {0xF0,52}, {0xF1,54}, {0xF2,56}, {0xF3,60}, {0xF4,64}, {0xF5,66}, {0xF6,68}, {0xF7,72},
    {0xF8,76}, {0xF9,78}, {0xFA,80}, {0xFB,84}, {0xFC,88}, {0xFD,90}, {0xFE,92}, {0xFF,96}
};

/*
 * public SequenceReader
 */

SequenceReader::SequenceReader(PlayerContext& ctx, uint8_t maxLoops) 
    : ctx(ctx), maxLoops(maxLoops)
{
}

void SequenceReader::Process()
{
    ctx.seq.bpmStack += uint32_t(float(ctx.seq.bpm) * speedFactor);
    while (ctx.seq.bpmStack >= BPM_PER_FRAME * INTERFRAMES) {
        processSequenceTick();
        ctx.seq.bpmStack -= BPM_PER_FRAME * INTERFRAMES;
    }
}

bool SequenceReader::EndReached() const
{
    return endReached;
}

void SequenceReader::Restart()
{
    endReached = false;
}

void SequenceReader::SetSpeedFactor(float speedFactor)
{
    this->speedFactor = speedFactor;
}

/*
 * private SequenceReader
 */

void SequenceReader::processSequenceTick()
{
    Rom& rom = Rom::Instance();
    // process all tracks
    bool isSongRunning = false;
    int ntrk = -1;
    for (Track& cTrk : ctx.seq.tracks) {
        ntrk += 1;
        if (!cTrk.isRunning)
            continue;

        isSongRunning = true;
        
        if (cTrk.mod > 0)
            cTrk.lfoPhase = uint8_t(cTrk.lfoPhase + cTrk.lfos);
        else
            cTrk.lfoPhase = 0; // if mod is 0, set phase to 0 too
        if (tickTrackNotes(uint8_t(ntrk), cTrk.activeNotes) > 0) {
            if (cTrk.lfodlCount > 0) {
                cTrk.lfodlCount--;
                cTrk.lfoPhase = 0;
            }
        } else
            cTrk.lfodlCount = cTrk.lfodl;
        if ((cTrk.lfodl == cTrk.lfodlCount && cTrk.lfodl != 0) || (cTrk.lfos == 0))
            cTrk.lfoPhase = 0;

        // count down last delay and process
        bool updatePV = false;
        if (--cTrk.delay <= 0) {
            while (cTrk.isRunning) {
                uint8_t cmd = rom.ReadU8(cTrk.pos++);
                // check if a previous command should be repeated
                if (cmd <= 0x7F) {
                    switch (cTrk.lastEvent) {
                        case LEvent::NONE:
                            break;
                        case LEvent::VOICE:
                            cTrk.prog = cmd;
                            break;
                        case LEvent::VOL:    
                            cTrk.vol = cmd;
                            updatePV = true;
                            break;
                        case LEvent::PAN:
                            cTrk.pan = int8_t(cmd - 0x40);
                            updatePV = true;
                            break;
                        case LEvent::BEND:
                            cTrk.bend = int8_t(cmd - 0x40);
                            updatePV = true;
                            break;
                        case LEvent::BENDR:
                            cTrk.bendr = cmd;
                            updatePV = true;
                            break;
                        case LEvent::MOD:
                            cTrk.mod = cmd;
                            updatePV = true;
                            break;
                        case LEvent::TUNE:
                            cTrk.tune = int8_t(cmd - 0x40);
                            updatePV = true;
                            break;
                        case LEvent::XCMD: 
                            {
                                uint8_t arg = rom.ReadU8(cTrk.pos++);
                                if (cmd == 0x8) {
                                    cTrk.echoVol = arg;
                                } else if (cmd == 0x9) {
                                    cTrk.echoLen = arg;
                                }
                            }
                            break;
                        case LEvent::NOTE:
                            {
                                uint8_t key = cTrk.lastNoteKey = uint8_t(cTrk.keyShift + int(cmd));
                                // if velocity parameter provided
                                if (rom.ReadU8(cTrk.pos) < 128) {
                                    // if gate parameter provided
                                    if (rom.ReadU8(cTrk.pos+1) < 128) {
                                        uint8_t vel = cTrk.lastNoteVel = rom.ReadU8(cTrk.pos++);
                                        int8_t len = static_cast<int8_t>(cTrk.lastNoteLen + rom.ReadU8(cTrk.pos++));
                                        playNote(cTrk, Note(key, vel, len), uint8_t(ntrk));
                                    } else {
                                        uint8_t vel = cTrk.lastNoteVel = rom.ReadU8(cTrk.pos++);
                                        int8_t len = cTrk.lastNoteLen;
                                        playNote(cTrk, Note(key, vel, len), uint8_t(ntrk));
                                    }
                                } else {
                                    playNote(cTrk, Note(key, cTrk.lastNoteVel, cTrk.lastNoteLen), uint8_t(ntrk));
                                }
                            }
                            break;
                        case LEvent::TIE:
                            {
                                uint8_t key = cTrk.lastNoteKey = uint8_t(cTrk.keyShift + int(cmd));
                                // if velocity parameter provided
                                if (rom.ReadU8(cTrk.pos) < 128) {
                                    uint8_t vel = cTrk.lastNoteVel = rom.ReadU8(cTrk.pos++);
                                    playNote(cTrk, Note(key, vel, NOTE_TIE), uint8_t(ntrk));
                                } else {
                                    playNote(cTrk, Note(key, cTrk.lastNoteVel, NOTE_TIE), uint8_t(ntrk));
                                }
                            }
                            break;
                        case LEvent::EOT:
                            {
                                uint8_t key = cTrk.lastNoteKey = uint8_t(cTrk.keyShift + int(cmd));
                                stopNote(key, uint8_t(ntrk));
                                cTrk.lastNoteKey = key;
                            }
                            break;
                        default: 
                            throw Xcept("Invalid Last Event");
                    } // end repeat command switch
                } else if (cmd == 0x80) {
                    // NOP delay
                } else if (cmd <= 0xB0) {
                    // normal delay
                    cTrk.delay = delayLut.at(cmd);
                    break;
                } else if (cmd <= 0xCF) {
                    // non note commands
                    switch (cmd) {
                        case 0xB1:
                            // FINE, end of track
                            cTrk.isRunning = false;
                            stopNote(NOTE_ALL, uint8_t(ntrk));
                            break;
                        case 0xB2:
                            // GOTO
                            if (ntrk == 0) {
                                if (maxLoops-- <= 0) {
                                    endReached = true;
                                    ctx.mixer.StartFadeOut(SONG_FADE_OUT_TIME);
                                }
                            }
                            cTrk.pos = rom.ReadAgbPtrToPos(cTrk.pos);
                            break;
                        case 0xB3:
                            // PATT, call sub
                            if (cTrk.reptCount > 0)
                                throw Xcept("Nested track calls are not allowed: 0x%7X", cTrk.pos);

                            cTrk.returnPos = cTrk.pos + 4;
                            cTrk.reptCount = 1;
                            cTrk.pos = cTrk.patBegin = rom.ReadAgbPtrToPos(cTrk.pos);
                            break;
                        case 0xB4:
                            // PEND, end of sub
                            if (cTrk.reptCount > 0) {
                                if (--cTrk.reptCount > 0) {
                                    cTrk.pos = cTrk.patBegin;
                                } else {
                                    cTrk.pos = cTrk.returnPos;
                                }
                            }
                            break;
                        case 0xB5:
                            // REPT
                            if (cTrk.reptCount > 0)
                                throw Xcept("Nested track calls are not allowed: 0x%7X", cTrk.pos);

                            cTrk.reptCount = rom.ReadU8(cTrk.pos++);
                            cTrk.returnPos = cTrk.pos + 4;
                            cTrk.pos = rom.ReadAgbPtrToPos(cTrk.pos);
                            break;
                        case 0xB9:
                            // MEMACC, not useful, get's ignored
                            cTrk.pos += 3;
                            break;
                        case 0xBA:
                            // PRIO, TODO actually do something with the prio
                            cTrk.prio = rom.ReadU8(cTrk.pos++);
                            break;
                        case 0xBB:
                            // TEMPO
                            ctx.seq.bpm = static_cast<uint16_t>(rom.ReadU8(cTrk.pos++) * 2);
                            break;
                        case 0xBC:
                            // KEYSH, transpose
                            cTrk.keyShift = rom.ReadS8(cTrk.pos++);
                            break;
                        case 0xBD:
                            // VOICE
                            cTrk.lastEvent = LEvent::VOICE;
                            cTrk.prog = rom.ReadU8(cTrk.pos++);
                            break;
                        case 0xBE:
                            // VOL
                            cTrk.lastEvent = LEvent::VOL;
                            cTrk.vol = rom.ReadU8(cTrk.pos++);
                            updatePV = true;
                            break;
                        case 0xBF:
                            // PAN
                            cTrk.lastEvent = LEvent::PAN;
                            cTrk.pan = static_cast<int8_t>(rom.ReadS8(cTrk.pos++) - 0x40);
                            updatePV = true;
                            break;
                        case 0xC0:
                            // BEND
                            cTrk.lastEvent = LEvent::BEND;
                            cTrk.bend = static_cast<int8_t>(rom.ReadS8(cTrk.pos++) - 0x40);
                            updatePV = true;
                            // update pitch
                            break;
                        case 0xC1:
                            // BENDR
                            cTrk.lastEvent = LEvent::BENDR;
                            cTrk.bendr = rom.ReadU8(cTrk.pos++);
                            updatePV = true;
                            // update pitch
                            break;
                        case 0xC2:
                            // LFOS
                            cTrk.lfos = rom.ReadU8(cTrk.pos++);
                            break;
                        case 0xC3:
                            // LFODL
                            cTrk.lfodlCount = cTrk.lfodl = rom.ReadU8(cTrk.pos++);
                            break;
                        case 0xC4:
                            // MOD
                            cTrk.lastEvent = LEvent::MOD;
                            cTrk.mod = rom.ReadU8(cTrk.pos++);
                            updatePV = true;
                            break;
                        case 0xC5:
                            // MODT
                            switch (rom.ReadU8(cTrk.pos++)) {
                                case 0: cTrk.modt = MODT::PITCH; break;
                                case 1: cTrk.modt = MODT::VOL; break;
                                case 2: cTrk.modt = MODT::PAN; break;
                                default: cTrk.modt = MODT::PITCH; break;
                            }
                            break;
                        case 0xC8:
                            // TUNE
                            cTrk.lastEvent = LEvent::TUNE;
                            cTrk.tune = static_cast<int8_t>(rom.ReadS8(cTrk.pos++) - 0x40);
                            updatePV = true;
                            break;
                        case 0xCD:
                            // XCMD
                            {
                                cTrk.lastEvent = LEvent::XCMD;
                                uint8_t type = rom.ReadU8(cTrk.pos++);
                                uint8_t arg = rom.ReadU8(cTrk.pos++);
                                if (type == 0x8) {
                                    cTrk.echoVol = arg;
                                } else if (type == 0x9) {
                                    cTrk.echoLen = arg;
                                }
                            }
                            break;
                        case 0xCE:
                            // EOT
                            {
                                cTrk.lastEvent = LEvent::EOT;
                                uint8_t next = rom.ReadU8(cTrk.pos);
                                if (next < 128) {
                                    stopNote(uint8_t(cTrk.keyShift + int(next)), uint8_t(ntrk));
                                    cTrk.lastNoteKey = uint8_t(cTrk.keyShift + int(next));
                                    cTrk.pos++;
                                } else {
                                    stopNote(cTrk.lastNoteKey, uint8_t(ntrk));
                                }
                            }
                            break;
                        case 0xCF:
                            // TIE
                            // cmd = note length
                            cTrk.lastEvent = LEvent::TIE;
                            if (rom.ReadU8(cTrk.pos) < 128) {
                                // new midi key
                                if (rom.ReadU8(cTrk.pos+1) < 128) {
                                    // new velocity
                                    uint8_t key = cTrk.lastNoteKey = static_cast<uint8_t>(
                                            (cTrk.keyShift + rom.ReadU8(cTrk.pos++)) % 128);
                                    uint8_t vel = cTrk.lastNoteVel = rom.ReadU8(cTrk.pos++);
                                    playNote(cTrk, Note(key, vel, NOTE_TIE), uint8_t(ntrk));
                                } else {
                                    // repeat velocity
                                    uint8_t key = cTrk.lastNoteKey = static_cast<uint8_t>(
                                            (cTrk.keyShift + rom.ReadU8(cTrk.pos++)) % 128);
                                    playNote(cTrk, Note(key, cTrk.lastNoteVel, NOTE_TIE), uint8_t(ntrk));
                                }
                            } else {
                                // repeat midi key
                                playNote(cTrk, Note(cTrk.lastNoteKey, cTrk.lastNoteVel, NOTE_TIE), uint8_t(ntrk));
                            }
                            break;
                        default:
                            throw Xcept("Unsupported command at 0x%7X: 0x%2X", (int)cTrk.pos, (int)cmd);
                    } // end main cmd switch
                } else {
                    // every other command is a note command
                    cTrk.lastEvent = LEvent::NOTE;
                    int8_t len = cTrk.lastNoteLen = noteLut.at(cmd);
                    // is midi key parameter provided?
                    if (rom.ReadU8(cTrk.pos) < 128) {
                        // is note volocity parameter provided?
                        if (rom.ReadU8(cTrk.pos+1) < 128) {
                            // is gate time parameter provided?
                            if (rom.ReadU8(cTrk.pos+2) < 128) {
                                // add gate time
                                uint8_t key = cTrk.lastNoteKey = static_cast<uint8_t>(
                                        (cTrk.keyShift + rom.ReadU8(cTrk.pos++)) % 128);
                                uint8_t vel = cTrk.lastNoteVel = rom.ReadU8(cTrk.pos++);
                                len = static_cast<int8_t>(len + rom.ReadU8(cTrk.pos++));
                                playNote(cTrk, Note(key, vel, len), uint8_t(ntrk));
                            } else {
                                // no gate time
                                uint8_t key = cTrk.lastNoteKey = static_cast<uint8_t>(
                                        (cTrk.keyShift + rom.ReadU8(cTrk.pos++)) % 128);
                                uint8_t vel = cTrk.lastNoteVel = rom.ReadU8(cTrk.pos++);
                                playNote(cTrk, Note(key, vel, len), uint8_t(ntrk));
                            }
                        } else {
                            // repeast note velocity
                            uint8_t key = cTrk.lastNoteKey = static_cast<uint8_t>(
                                    (cTrk.keyShift + rom.ReadU8(cTrk.pos++)) % 128);
                            playNote(cTrk, Note(key, cTrk.lastNoteVel, len), uint8_t(ntrk));
                        }
                    } else {
                        // repeat midi key
                        playNote(cTrk, Note(cTrk.lastNoteKey, cTrk.lastNoteVel, len), uint8_t(ntrk));
                    }
                }
            } // end of processing loop
        } // end of single tick processing handler
        if (updatePV || cTrk.mod > 0) {
            setTrackPV(uint8_t(ntrk), 
                    cTrk.GetVol(),
                    cTrk.GetPan(),
                    cTrk.pitch = cTrk.GetPitch());
        } else {
            cTrk.pitch = cTrk.GetPitch();
        }
    } // end of track iteration
    if (!isSongRunning && !endReached) {
        ctx.mixer.StartFadeOut(SONG_FINISH_TIME);
        endReached = true;
    }
} // end processSequenceTick

void SequenceReader::playNote(Track& trk, Note note, uint8_t owner)
{
    if (trk.prog > 127)
        return;

    CGBPolyphony cgbPolyphony = ConfigManager::Instance().GetCgbPolyphony();

    auto cgbPolyphonySuppressFunc = [&](auto& channels) {
        if (cgbPolyphony == CGBPolyphony::MONO_STRICT) {
            channels.clear();
        } else if (cgbPolyphony == CGBPolyphony::MONO_SMOOTH) {
            for (auto& chn : channels)
                chn.Release(true);
        }
    };

    uint8_t oldKey = note.midiKey;
    note.midiKey = ctx.bnk.GetMidiKey(trk.prog, oldKey);
    switch (ctx.bnk.GetInstrType(trk.prog, oldKey)) {
        case InstrType::PCM:
            {
                uint8_t pan = ctx.bnk.GetPan(trk.prog, oldKey);
                ctx.sndChannels.emplace_back(
                        owner,
                        ctx.bnk.GetSampInfo(trk.prog, oldKey),
                        ctx.bnk.GetADSR(trk.prog, oldKey),
                        note,
                        trk.GetVol(),
                        (pan & 0x80) ? int8_t(int(pan) - 0xC0) : trk.GetPan(),
                        trk.GetPitch(),
                        false);
            }
            break;
        case InstrType::PCM_FIXED:
            {
                uint8_t pan = ctx.bnk.GetPan(trk.prog, oldKey);
                ctx.sndChannels.emplace_back(
                        owner,
                        ctx.bnk.GetSampInfo(trk.prog, oldKey),
                        ctx.bnk.GetADSR(trk.prog, oldKey),
                        note,
                        trk.GetVol(),
                        (pan & 0x80) ? int8_t(int(pan) - 0xC0) : trk.GetPan(),
                        trk.GetPitch(),
                        true);
            }
            break;
        case InstrType::SQ1:
            // TODO Does pan of drum tables really only affect PCM channels?
            cgbPolyphonySuppressFunc(ctx.sq1Channels);
            ctx.sq1Channels.emplace_back(
                    owner, 
                    ctx.bnk.GetCGBDef(trk.prog, oldKey).wd,
                    ctx.bnk.GetADSR(trk.prog, oldKey),
                    note, 
                    trk.GetVol(), 
                    trk.GetPan(), 
                    trk.GetPitch());
            break;
        case InstrType::SQ2:
            cgbPolyphonySuppressFunc(ctx.sq2Channels);
            ctx.sq2Channels.emplace_back(
                    owner, 
                    ctx.bnk.GetCGBDef(trk.prog, oldKey).wd,
                    ctx.bnk.GetADSR(trk.prog, oldKey),
                    note, 
                    trk.GetVol(), 
                    trk.GetPan(), 
                    trk.GetPitch());
            break;
        case InstrType::WAVE:
            cgbPolyphonySuppressFunc(ctx.waveChannels);
            ctx.waveChannels.emplace_back(
                    owner, 
                    ctx.bnk.GetCGBDef(trk.prog, oldKey).wavePtr,
                    ctx.bnk.GetADSR(trk.prog, oldKey),
                    note,
                    trk.GetVol(),
                    trk.GetPan(),
                    trk.GetPitch());
            break;
        case InstrType::NOISE:
            cgbPolyphonySuppressFunc(ctx.noiseChannels);
            ctx.noiseChannels.emplace_back(
                    owner, 
                    ctx.bnk.GetCGBDef(trk.prog, oldKey).np,
                    ctx.bnk.GetADSR(trk.prog, oldKey),
                    note, 
                    trk.GetVol(),
                    trk.GetPan(),
                    trk.GetPitch());
            break;
        case InstrType::INVALID:
            return;
    }
}

void SequenceReader::stopNote(uint8_t key, uint8_t owner)
{
    auto stopNotes = [&](auto& channels) {
        for (auto& chn : channels) {
            if (chn.GetOwner() == owner && (
                        key == NOTE_ALL || (
                            chn.GetMidiKey() == key &&
                            chn.GetNoteLength() == NOTE_TIE))) {
                chn.Release();
            }
        }
    };

    stopNotes(ctx.sndChannels);
    stopNotes(ctx.sq1Channels);
    stopNotes(ctx.sq2Channels);
    stopNotes(ctx.waveChannels);
    stopNotes(ctx.noiseChannels);
}

int SequenceReader::tickTrackNotes(uint8_t owner, std::bitset<NUM_NOTES>& activeNotes)
{
    std::bitset<128> backBuffer;
    int active = 0;

    auto tickFunc = [&](auto& channels) {
        for (auto& chn : channels) {
            if (chn.GetOwner() == owner) {
                if (chn.TickNote()) {
                    active++;
                    backBuffer[chn.GetMidiKey() % 128] = true;
                }
            }
        }
    };

    tickFunc(ctx.sndChannels);
    tickFunc(ctx.sq1Channels);
    tickFunc(ctx.sq2Channels);
    tickFunc(ctx.waveChannels);
    tickFunc(ctx.noiseChannels);

    activeNotes = backBuffer;
    return active;
}

void SequenceReader::setTrackPV(uint8_t owner, uint8_t vol, int8_t pan, int16_t pitch)
{
    auto setFunc = [&](auto& channels) {
        for (auto& chn : channels) {
            if (chn.GetOwner() == owner) {
                chn.SetVol(vol, pan);
                chn.SetPitch(pitch);
            }
        }
    };

    setFunc(ctx.sndChannels);
    setFunc(ctx.sq1Channels);
    setFunc(ctx.sq2Channels);
    setFunc(ctx.waveChannels);
    setFunc(ctx.noiseChannels);
}
