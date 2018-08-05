#include <cmath>

#include "StreamGenerator.h"
#include "Xcept.h"
#include "Util.h"
#include "Debug.h"

#define SONG_FADE_OUT_TIME 10000
#define SONG_FINISH_TIME 1000

using namespace std;
using namespace agbplay;


/*
 * public EnginePars
 */

EnginePars::EnginePars(uint8_t vol, uint8_t rev, uint8_t freq)
{
    this->vol = vol;
    this->rev = rev;
    this->freq = freq;
}

EnginePars::EnginePars()
{
}

/*
 * public StreamGenerator
 */

const vector<uint32_t> StreamGenerator::freqLut = {
    5734, 7884, 10512, 13379,
    15768, 18157, 21024, 26758,
    31536, 36314, 40137, 42048
};

const map<uint8_t, int8_t> StreamGenerator::delayLut = {
    {0x81,1 }, {0x82,2 }, {0x83,3 }, {0x84,4 }, {0x85,5 }, {0x86,6 }, {0x87,7 }, {0x88,8 },
    {0x89,9 }, {0x8A,10}, {0x8B,11}, {0x8C,12}, {0x8D,13}, {0x8E,14}, {0x8F,15}, {0x90,16},
    {0x91,17}, {0x92,18}, {0x93,19}, {0x94,20}, {0x95,21}, {0x96,22}, {0x97,23}, {0x98,24},
    {0x99,28}, {0x9A,30}, {0x9B,32}, {0x9C,36}, {0x9D,40}, {0x9E,42}, {0x9F,44}, {0xA0,48},
    {0xA1,52}, {0xA2,54}, {0xA3,56}, {0xA4,60}, {0xA5,64}, {0xA6,66}, {0xA7,68}, {0xA8,72},
    {0xA9,76}, {0xAA,78}, {0xAB,80}, {0xAC,84}, {0xAD,88}, {0xAE,90}, {0xAF,92}, {0xB0,96}
};

const map<uint8_t, int8_t> StreamGenerator::noteLut = {
    {0xD0,1 }, {0xD1,2 }, {0xD2,3 }, {0xD3,4 }, {0xD4,5 }, {0xD5,6 }, {0xD6,7 }, {0xD7,8 },
    {0xD8,9 }, {0xD9,10}, {0xDA,11}, {0xDB,12}, {0xDC,13}, {0xDD,14}, {0xDE,15}, {0xDF,16},
    {0xE0,17}, {0xE1,18}, {0xE2,19}, {0xE3,20}, {0xE4,21}, {0xE5,22}, {0xE6,23}, {0xE7,24},
    {0xE8,28}, {0xE9,30}, {0xEA,32}, {0xEB,36}, {0xEC,40}, {0xED,42}, {0xEE,44}, {0xEF,48},
    {0xF0,52}, {0xF1,54}, {0xF2,56}, {0xF3,60}, {0xF4,64}, {0xF5,66}, {0xF6,68}, {0xF7,72},
    {0xF8,76}, {0xF9,78}, {0xFA,80}, {0xFB,84}, {0xFC,88}, {0xFD,90}, {0xFE,92}, {0xFF,96}
};

StreamGenerator::StreamGenerator(Sequence& seq, EnginePars ep, uint8_t maxLoops, float speedFactor, ReverbType rtype)
: seq(seq), sbnk(seq.GetRom(), seq.GetSndBnk()),
    sm(freqLut[clip<uint8_t>(0, uint8_t(ep.freq-1), 11)], 10548,
            (ep.rev >= 0x80) ? ep.rev & 0x7F : seq.GetReverb() & 0x7F,
            float(ep.vol + 1) / 16.0f,
            rtype, (uint8_t)seq.tracks.size())
{
    this->ep = ep;
    this->maxLoops = maxLoops;
    this->speedFactor = speedFactor;
    this->isEnding = false;
}

StreamGenerator::~StreamGenerator()
{
}

size_t StreamGenerator::GetBufferUnitCount()
{
    return sm.GetBufferUnitCount();
}

size_t StreamGenerator::GetActiveChannelCount()
{
    return sm.GetActiveChannelCount();
}

uint32_t StreamGenerator::GetRenderSampleRate()
{
    return sm.GetRenderSampleRate();
}

vector<vector<float>>& StreamGenerator::ProcessAndGetAudio()
{
    processSequenceFrame();
    return sm.ProcessAndGetAudio();
}

bool StreamGenerator::HasStreamEnded()
{
    return isEnding && sm.IsFadeDone();
}

Sequence& StreamGenerator::GetWorkingSequence()
{
    return seq;
}

void StreamGenerator::SetSpeedFactor(float speedFactor)
{
    this->speedFactor = speedFactor;
}

/*
 * private StreamGenerator
 */

void StreamGenerator::processSequenceFrame()
{
    seq.bpmStack += uint32_t(float(seq.bpm) * speedFactor);
    while (seq.bpmStack >= BPM_PER_FRAME * INTERFRAMES) {
        processSequenceTick();
        seq.bpmStack -= BPM_PER_FRAME * INTERFRAMES;
    }
}

void StreamGenerator::processSequenceTick()
{
    Rom& reader = seq.GetRom();
    // process all tracks
    bool isSongRunning = false;
    int ntrk = 0;
    for (Sequence::Track& cTrk : seq.tracks) {
        if (!cTrk.isRunning)
            continue;

        isSongRunning = true;

        if (sm.TickTrackNotes(uint8_t(ntrk), cTrk.activeNotes) > 0) {
            if (cTrk.lfodlCount > 0) {
                cTrk.lfodlCount--;
                cTrk.lfoPhase = 0;
            } else {
                cTrk.lfoPhase = uint8_t(cTrk.lfoPhase + cTrk.lfos);
            }
        } else {
            cTrk.lfoPhase = 0;
            cTrk.lfodlCount = cTrk.lfodl;
        }
        // count down last delay and process
        bool updatePV = false;
        if (--cTrk.delay <= 0) {
            while (cTrk.isRunning) {
                uint8_t cmd = reader[cTrk.pos++];
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
                                uint8_t arg = reader[cTrk.pos++];
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
                                if (reader[cTrk.pos] < 128) {
                                    // if gate parameter provided
                                    if (reader[cTrk.pos+1] <= 3 && reader[cTrk.pos+1] >= 1) {
                                        uint8_t vel = cTrk.lastNoteVel = reader[cTrk.pos++];
                                        int8_t len = int8_t(cTrk.lastNoteLen + reader[cTrk.pos++]);
                                        playNote(cTrk, Note(key, vel, len), uint8_t(ntrk));
                                    } else {
                                        uint8_t vel = cTrk.lastNoteVel = reader[cTrk.pos++];
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
                                if (reader[cTrk.pos] < 128) {
                                    uint8_t vel = cTrk.lastNoteVel = reader[cTrk.pos++];
                                    playNote(cTrk, Note(key, vel, NOTE_TIE), uint8_t(ntrk));
                                } else {
                                    playNote(cTrk, Note(key, cTrk.lastNoteVel, NOTE_TIE), uint8_t(ntrk));
                                }
                            }
                            break;
                        case LEvent::EOT:
                            {
                                uint8_t key = cTrk.lastNoteKey = uint8_t(cTrk.keyShift + int(cmd));
                                sm.StopChannel(uint8_t(ntrk), key);
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
                            sm.StopChannel(uint8_t(ntrk), NOTE_ALL);
                            break;
                        case 0xB2:
                            // GOTO
                            if (ntrk == 0) {
                                if (maxLoops-- <= 0) {
                                    isEnding = true;
                                    sm.FadeOut(SONG_FADE_OUT_TIME);
                                }
                            }
                            cTrk.pos = reader.AGBPtrToPos(*(uint32_t *)&reader[cTrk.pos]);
                            break;
                        case 0xB3:
                            // PATT, call sub
                            if (cTrk.reptCount > 0)
                                throw Xcept("Nested track calls are not allowed: 0x%7X", cTrk.pos);

                            cTrk.returnPos = cTrk.pos + 4;
                            cTrk.reptCount = 1;
                            cTrk.pos = cTrk.patBegin = reader.AGBPtrToPos(*(uint32_t *)&reader[cTrk.pos]);
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

                            cTrk.reptCount = reader[cTrk.pos++];
                            cTrk.returnPos = cTrk.pos + 4;
                            cTrk.pos = reader.AGBPtrToPos(*(uint32_t *)&reader[cTrk.pos]);
                            break;
                        case 0xB9:
                            // MEMACC, not useful, get's ignored
                            cTrk.pos += 3;
                            break;
                        case 0xBA:
                            // PRIO, TODO actually do something with the prio
                            cTrk.prio = reader[cTrk.pos++];
                            break;
                        case 0xBB:
                            // TEMPO
                            seq.bpm = uint16_t(reader[cTrk.pos++] * 2);
                            break;
                        case 0xBC:
                            // KEYSH, transpose
                            cTrk.keyShift = int8_t(reader[cTrk.pos++]);
                            break;
                        case 0xBD:
                            // VOICE
                            cTrk.lastEvent = LEvent::VOICE;
                            cTrk.prog = reader[cTrk.pos++];
                            break;
                        case 0xBE:
                            // VOL
                            cTrk.lastEvent = LEvent::VOL;
                            cTrk.vol = reader[cTrk.pos++];
                            updatePV = true;
                            break;
                        case 0xBF:
                            // PAN
                            cTrk.lastEvent = LEvent::PAN;
                            cTrk.pan = int8_t(reader[cTrk.pos++] - 0x40);
                            updatePV = true;
                            break;
                        case 0xC0:
                            // BEND
                            cTrk.lastEvent = LEvent::BEND;
                            cTrk.bend = int8_t(reader[cTrk.pos++] - 0x40);
                            updatePV = true;
                            // update pitch
                            break;
                        case 0xC1:
                            // BENDR
                            cTrk.lastEvent = LEvent::BENDR;
                            cTrk.bendr = reader[cTrk.pos++];
                            updatePV = true;
                            // update pitch
                            break;
                        case 0xC2:
                            // LFOS
                            cTrk.lfos = reader[cTrk.pos++];
                            break;
                        case 0xC3:
                            // LFODL
                            cTrk.lfodlCount = cTrk.lfodl = reader[cTrk.pos++];
                            break;
                        case 0xC4:
                            // MOD
                            cTrk.lastEvent = LEvent::MOD;
                            cTrk.mod = reader[cTrk.pos++];
                            updatePV = true;
                            break;
                        case 0xC5:
                            // MODT
                            switch (reader[cTrk.pos++]) {
                                case 0: cTrk.modt = MODT::PITCH; break;
                                case 1: cTrk.modt = MODT::VOL; break;
                                case 2: cTrk.modt = MODT::PAN; break;
                                default: cTrk.modt = MODT::PITCH; break;
                            }
                            break;
                        case 0xC8:
                            // TUNE
                            cTrk.lastEvent = LEvent::TUNE;
                            cTrk.tune = int8_t(reader[cTrk.pos++] - 0x40);
                            updatePV = true;
                            break;
                        case 0xCD:
                            // XCMD
                            {
                                cTrk.lastEvent = LEvent::XCMD;
                                uint8_t type = reader[cTrk.pos++];
                                uint8_t arg = reader[cTrk.pos++];
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
                                uint8_t next = reader[cTrk.pos];
                                if (next < 128) {
                                    sm.StopChannel(uint8_t(ntrk), uint8_t(cTrk.keyShift + int(next)));
                                    cTrk.lastNoteKey = uint8_t(cTrk.keyShift + int(next));
                                    cTrk.pos++;
                                } else {
                                    sm.StopChannel(uint8_t(ntrk), cTrk.lastNoteKey);
                                }
                            }
                            break;
                        case 0xCF:
                            // TIE
                            // cmd = note length
                            cTrk.lastEvent = LEvent::TIE;
                            if (reader[cTrk.pos] < 128) {
                                // new midi key
                                if (reader[cTrk.pos+1] < 128) {
                                    // new velocity
                                    uint8_t key = cTrk.lastNoteKey = uint8_t(cTrk.keyShift + int(reader[cTrk.pos++]));
                                    uint8_t vel = cTrk.lastNoteVel = reader[cTrk.pos++];
                                    playNote(cTrk, Note(key, vel, NOTE_TIE), uint8_t(ntrk));
                                } else {
                                    // repeat velocity
                                    uint8_t key = cTrk.lastNoteKey = uint8_t(cTrk.keyShift + int(reader[cTrk.pos++]));
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
                    if (reader[cTrk.pos] < 128) {
                        // is note volocity parameter provided?
                        if (reader[cTrk.pos+1] < 128) {
                            // is gate time parameter provided?
                            if (reader[cTrk.pos+2] <= 3 && reader[cTrk.pos+2] >= 1) {
                                // add gate time
                                uint8_t key = cTrk.lastNoteKey = uint8_t(cTrk.keyShift + int(reader[cTrk.pos++]));
                                uint8_t vel = cTrk.lastNoteVel = reader[cTrk.pos++];
                                len = int8_t(len + reader[cTrk.pos++]);
                                playNote(cTrk, Note(key, vel, len), uint8_t(ntrk));
                            } else {
                                // no gate time
                                uint8_t key = cTrk.lastNoteKey = uint8_t(cTrk.keyShift + int(reader[cTrk.pos++]));
                                uint8_t vel = cTrk.lastNoteVel = reader[cTrk.pos++];
                                playNote(cTrk, Note(key, vel, len), uint8_t(ntrk));
                            }
                        } else {
                            // repeast note velocity
                            uint8_t key = cTrk.lastNoteKey = uint8_t(cTrk.keyShift + int(reader[cTrk.pos++]));
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
            sm.SetTrackPV(uint8_t(ntrk),
                    cTrk.GetVol(),
                    cTrk.GetPan(),
                    cTrk.pitch = cTrk.GetPitch());
        } else {
            cTrk.pitch = cTrk.GetPitch();
        }
        ntrk++;
    } // end of track iteration
    if (!isSongRunning && !isEnding) {
        sm.FadeOut(SONG_FINISH_TIME);
        isEnding = true;
    }
} // end processSequenceTick

void StreamGenerator::playNote(Sequence::Track& trk, Note note, uint8_t owner)
{
    if (trk.prog > 127)
        return;

    uint8_t oldKey = note.midiKey;
    note.midiKey = sbnk.GetMidiKey(trk.prog, oldKey);
    switch (sbnk.GetInstrType(trk.prog, oldKey)) {
        case InstrType::PCM:
            {
                uint8_t pan = sbnk.GetPan(trk.prog, oldKey);
                sm.NewSoundChannel(
                        owner,
                        sbnk.GetSampInfo(trk.prog, oldKey),
                        sbnk.GetADSR(trk.prog, oldKey),
                        note,
                        trk.GetVol(),
                        (pan & 0x80) ? int8_t(int(pan) - 0xC0) : trk.GetPan(),
                        trk.GetPitch(),
                        false);
            }
            break;
        case InstrType::PCM_FIXED:
            {
                uint8_t pan = sbnk.GetPan(trk.prog, oldKey);
                sm.NewSoundChannel(
                        owner,
                        sbnk.GetSampInfo(trk.prog, oldKey),
                        sbnk.GetADSR(trk.prog, oldKey),
                        note,
                        trk.GetVol(),
                        (pan & 0x80) ? int8_t(int(pan) - 0xC0) : trk.GetPan(),
                        trk.GetPitch(),
                        true);
            }
            break;
        case InstrType::SQ1:
            sm.NewCGBNote(
                    owner,
                    sbnk.GetCGBDef(trk.prog, oldKey),
                    sbnk.GetADSR(trk.prog, oldKey),
                    note,
                    trk.GetVol(),
                    trk.GetPan(),
                    trk.GetPitch(),
                    CGBType::SQ1);
            break;
        case InstrType::SQ2:
            sm.NewCGBNote(
                    owner,
                    sbnk.GetCGBDef(trk.prog, oldKey),
                    sbnk.GetADSR(trk.prog, oldKey),
                    note,
                    trk.GetVol(),
                    trk.GetPan(),
                    trk.GetPitch(),
                    CGBType::SQ2);
            break;
        case InstrType::WAVE:
            sm.NewCGBNote(
                    owner,
                    sbnk.GetCGBDef(trk.prog, oldKey),
                    sbnk.GetADSR(trk.prog, oldKey),
                    note,
                    trk.GetVol(),
                    trk.GetPan(),
                    trk.GetPitch(),
                    CGBType::WAVE);
            break;
        case InstrType::NOISE:
            sm.NewCGBNote(
                    owner,
                    sbnk.GetCGBDef(trk.prog, oldKey),
                    sbnk.GetADSR(trk.prog, oldKey),
                    note,
                    trk.GetVol(),
                    trk.GetPan(),
                    trk.GetPitch(),
                    CGBType::NOISE);
            break;
        case InstrType::INVALID:
            return;
    }
}
