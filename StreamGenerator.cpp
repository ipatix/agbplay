#include <cmath>

#include "StreamGenerator.h"
#include "MyException.h"
#include "Util.h"

#define SONG_FADE_OUT_TIME 7000
#define SONG_FINISH_TIME 2000

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

const map<uint8_t, int8_t> StreamGenerator::delayLut = {
    {0x81,1 }, {0x82,2 }, {0x83,3 }, {0x84,4 }, {0x85,5 }, {0x86,6 }, {0x87,7 }, {0x88,8 },
    {0x89,9 }, {0x8A,10}, {0x8B,11}, {0x8C,12}, {0x8D,13}, {0x8E,14}, {0x8F,15}, {0x90,16},
    {0x91,17}, {0x92,18}, {0x93,19}, {0x94,20}, {0x95,21}, {0x96,22}, {0x97,23}, {0x98,24},
    {0x99,28}, {0x9A,30}, {0x9B,32}, {0x9C,36}, {0x9D,40}, {0x9E,42}, {0x9F,44}, {0xA0,48},
    {0xA1,52}, {0xA2,52}, {0xA3,56}, {0xA4,60}, {0xA5,64}, {0xA6,66}, {0xA7,68}, {0xA8,72},
    {0xA9,76}, {0xAA,78}, {0xAB,80}, {0xAC,84}, {0xAD,88}, {0xAE,90}, {0xAF,92}, {0xB0,96}
};

const map<uint8_t, int8_t> StreamGenerator::noteLut = {
    {0xD0,1 }, {0xD1,2 }, {0xD2,3 }, {0xD3,4 }, {0xD4,5 }, {0xD5,6 }, {0xD6,7 }, {0xD7,8 },
    {0xD8,9 }, {0xD9,10}, {0xDA,11}, {0xDB,12}, {0xDC,13}, {0xDD,14}, {0xDE,15}, {0xDF,16},
    {0xE0,17}, {0xE1,18}, {0xE2,19}, {0xE3,20}, {0xE4,21}, {0xE5,22}, {0xE6,23}, {0xE7,24},
    {0xE8,28}, {0xE9,30}, {0xEA,32}, {0xEB,36}, {0xEC,40}, {0xED,42}, {0xEE,44}, {0xEF,48},
    {0xF0,52}, {0xF1,52}, {0xF2,56}, {0xF3,60}, {0xF4,64}, {0xF5,66}, {0xF6,68}, {0xF7,72},
    {0xF8,76}, {0xF9,78}, {0xFA,80}, {0xFB,84}, {0xFC,88}, {0xFD,90}, {0xFE,92}, {0xFF,96}
};

StreamGenerator::StreamGenerator(Sequence& seq, uint32_t fixedModeRate, EnginePars ep, uint8_t maxLoops) : seq(seq), sbnk(seq.GetRom(), seq.GetSndBnk()), sm(48000, fixedModeRate, seq.GetReverb() & 0x7F)
{
    this->ep = ep;
    this->maxLoops = maxLoops;
    this->isEnding = false;
}

StreamGenerator::~StreamGenerator()
{
}

uint32_t StreamGenerator::GetBufferUnitCount()
{
    return sm.GetBufferUnitCount();
}

float *StreamGenerator::ProcessAndGetAudio()
{
    processSequenceFrame();
    float *processedData = sm.ProcessAndGetAudio();
    if (isEnding && sm.IsFadeDone())
        return nullptr;
    else
        return processedData;
}

/*
 * private StreamGenerator
 */

void StreamGenerator::processSequenceFrame()
{
    while (seq.bpmStack >= 0) {
        processSequenceTick();
        seq.bpmStack -= seq.bpm * INTERFRAMES;
    }
    seq.bpmStack += BPM_PER_FRAME;
}

void StreamGenerator::processSequenceTick()
{
    Rom& reader = seq.GetRom();
    // process all tracks
    bool isSongRunning = false;
    bool isFirst = true;
    for (Sequence::Track& cTrk : seq.tracks) {
        if (!cTrk.isRunning)
            continue;

        isSongRunning = true;
        
        if (sm.TickTrackNotes((void *)&cTrk) > 0) {
            if (cTrk.lfodlCount > 0) {
                cTrk.lfodlCount--;
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
            while (true) {
                reader.Seek(cTrk.pos);
                uint8_t cmd = reader.ReadUInt8();
                if (cmd <= 0x7F) {
                    switch (cTrk.lastEvent) {
                        case LEvent::NONE:
                            cTrk.pos += 1;
                            break;
                        case LEvent::VOL:    
                            cTrk.vol = cmd;
                            cTrk.pos += 1;
                            break;
                        case LEvent::PAN:
                            cTrk.pan = int8_t(cmd - 0x40);
                            cTrk.pos += 1;
                            break;
                        case LEvent::BEND:
                            cTrk.bend = int8_t(cmd - 0x40);
                            cTrk.pos += 1;
                            break;
                        case LEvent::BENDR:
                            cTrk.bendr = cmd;
                            cTrk.pos += 1;
                            break;
                        case LEvent::MOD:
                            cTrk.mod = cmd;
                            cTrk.pos += 1;
                            break;
                        case LEvent::TUNE:
                            cTrk.tune = int8_t(cmd - 0x40);
                            cTrk.pos += 1;
                            break;
                        case LEvent::XCMD: 
                            {
                                uint8_t arg = reader.ReadUInt8();
                                if (cmd == 0x8) {
                                    cTrk.echoVol = arg;
                                } else if (cmd == 0x9) {
                                    cTrk.echoLen = arg;
                                }
                                cTrk.pos += 2;
                            }
                            break;
                        case LEvent::NOTE:
                            if (reader.PeekInt8(0) >= 0) {
                                if (reader.PeekInt8(1) >= 0) {
                                    uint8_t vel = cTrk.lastNoteVel = reader.ReadUInt8();
                                    int8_t len = int8_t(cTrk.lastNoteLen + reader.ReadInt8());
                                    playNote(cTrk, Note(cmd, vel, len), &cTrk);
                                    cTrk.pos += 3;
                                } else {
                                    uint8_t vel = cTrk.lastNoteVel = reader.ReadUInt8();
                                    int8_t len = cTrk.lastNoteLen;
                                    playNote(cTrk, Note(cmd, vel, len), &cTrk);
                                    cTrk.pos += 2;
                                }
                            } else {
                                playNote(cTrk, Note(cmd, cTrk.lastNoteVel, cTrk.lastNoteLen), &cTrk);
                                cTrk.pos += 1;
                            }
                            cTrk.lastNoteKey = cmd;
                            break;
                        case LEvent::TIE:
                            if (reader.PeekInt8(0) >= 0) {
                                uint8_t vel = cTrk.lastNoteVel = reader.ReadUInt8();
                                playNote(cTrk, Note(cmd, vel, NOTE_TIE), &cTrk);
                                cTrk.pos += 2;
                            } else {
                                playNote(cTrk, Note(cmd, cTrk.lastNoteVel, NOTE_TIE), &cTrk);
                                cTrk.pos += 1;
                            }
                            cTrk.lastNoteKey = cmd;
                            break;
                        case LEvent::EOT:
                            sm.StopChannel(&cTrk, cmd);
                            cTrk.pos += 1;
                            break;
                        default: 
                            break;
                    } // end repeat command switch
                } else if (cmd == 0x80) {
                    // NOP delay
                    cTrk.pos++;
                    continue;
                } else if (cmd <= 0xB0) {
                    // normal delay
                    cTrk.pos++;
                    cTrk.delay = delayLut.at(cmd);
                    break;
                } else if (cmd <= 0xCF) {
                    switch (cmd) {
                        case 0xB1:
                            // FINE, end of track
                            cTrk.isRunning = false;
                            cTrk.pos++;
                            break;
                        case 0xB2:
                            // GOTO
                            if (isFirst) {
                                if (maxLoops-- <= 0) {
                                    isEnding = true;
                                    sm.FadeOut(SONG_FADE_OUT_TIME);
                                }
                            }
                            cTrk.pos = reader.ReadAGBPtrToPos();
                            break;
                        case 0xB3:
                            // PATT, call sub
                            if (cTrk.isCalling)
                                throw MyException(FormatString("Nested track calls are not allowed: 0x%7X", cTrk.pos));

                            cTrk.returnPos = cTrk.pos + 5;
                            cTrk.reptCount = 1;
                            cTrk.pos = cTrk.patBegin = reader.ReadAGBPtrToPos();
                            break;
                        case 0xB4:
                            // PEND, end of sub
                            if (!cTrk.isCalling) {
                                // nothing is called so ignore it
                                cTrk.pos++;
                            } else {
                                if (--cTrk.reptCount > 0) {
                                    cTrk.pos = cTrk.patBegin;
                                } else {
                                    cTrk.pos = cTrk.returnPos;
                                    cTrk.isCalling = false;
                                }
                            }
                            break;
                        case 0xB5:
                            // REPT
                            if (cTrk.isCalling)
                                throw MyException(FormatString("Nested track calls are not allowed: 0x%7X", cTrk.pos));

                            cTrk.returnPos = cTrk.pos + 5;
                            cTrk.reptCount = reader.ReadUInt8();
                            cTrk.pos = reader.ReadAGBPtrToPos();
                            break;
                        case 0xBB:
                            // TEMPO
                            seq.bpm = uint16_t(reader.ReadUInt8() * 2);
                            cTrk.pos += 2;
                            break;
                        case 0xBC:
                            // KEYSH, transpose
                            cTrk.keyShift = reader.ReadInt8();
                            cTrk.pos += 2;
                            break;
                        case 0xBD:
                            // VOICE
                            cTrk.prog = reader.ReadUInt8();
                            cTrk.pos += 2;
                            break;
                        case 0xBE:
                            // VOL
                            cTrk.lastEvent = LEvent::VOL;
                            cTrk.vol = reader.ReadUInt8();
                            cTrk.pos += 2;
                            updatePV = true;
                            break;
                        case 0xBF:
                            // PAN
                            cTrk.lastEvent = LEvent::PAN;
                            cTrk.pan = int8_t(reader.ReadUInt8() - 0x40);
                            cTrk.pos += 2;
                            updatePV = true;
                            break;
                        case 0xC0:
                            // BEND
                            cTrk.lastEvent = LEvent::BEND;
                            cTrk.bend = int8_t(reader.ReadUInt8() - 0x40);
                            cTrk.pos += 2;
                            updatePV = true;
                            // update pitch
                            break;
                        case 0xC1:
                            // BENDR
                            cTrk.lastEvent = LEvent::BENDR;
                            cTrk.bendr = reader.ReadUInt8();
                            cTrk.pos += 2;
                            updatePV = true;
                            // update pitch
                            break;
                        case 0xC2:
                            // LFOS
                            cTrk.lfos = reader.ReadUInt8();
                            cTrk.pos += 2;
                            break;
                        case 0xC3:
                            // LFODL
                            cTrk.lfodl = reader.ReadUInt8();
                            cTrk.pos += 2;
                            break;
                        case 0xC4:
                            // MOD
                            cTrk.lastEvent = LEvent::MOD;
                            cTrk.mod = reader.ReadUInt8();
                            cTrk.pos += 2;
                            break;
                        case 0xC5:
                            // MODT
                            switch (reader.ReadUInt8()) {
                                case 0: cTrk.modt = MODT::PITCH; break;
                                case 1: cTrk.modt = MODT::VOL; break;
                                case 2: cTrk.modt = MODT::PAN; break;
                                default: cTrk.modt = MODT::PITCH; break;
                            }
                            cTrk.pos += 2;
                            break;
                        case 0xC8:
                            // TUNE
                            cTrk.lastEvent = LEvent::TUNE;
                            cTrk.tune = int8_t(reader.ReadUInt8() - 128);
                            cTrk.pos += 2;
                            updatePV = true;
                            break;
                        case 0xCD:
                            // XCMD
                            {
                                uint8_t type = reader.ReadUInt8();
                                uint8_t arg = reader.ReadUInt8();
                                if (type == 0x8) {
                                    cTrk.echoVol = arg;
                                } else if (type == 0x9) {
                                    cTrk.echoLen = arg;
                                }
                                cTrk.pos += 3;
                            }
                            break;
                        case 0xCE:
                            // EOT
                            cTrk.lastEvent = LEvent::EOT;
                            sm.StopChannel(&cTrk, reader.ReadUInt8());
                            cTrk.pos += 2;
                            break;
                        case 0xCF:
                            // TIE
                            cTrk.lastEvent = LEvent::TIE;
                            if (reader.PeekInt8(0) >= 0) {
                                // new midi key
                                if (reader.PeekInt8(1) >= 0) {
                                    // new velocity
                                    uint8_t key = cTrk.lastNoteKey = reader.ReadUInt8();
                                    uint8_t vel = cTrk.lastNoteVel = reader.ReadUInt8();
                                    playNote(cTrk, Note(key, vel, NOTE_TIE), &cTrk);
                                    cTrk.pos += 3;
                                } else {
                                    // repeat velocity
                                    uint8_t key = cTrk.lastNoteKey = reader.ReadUInt8();
                                    playNote(cTrk, Note(key, cTrk.lastNoteVel, NOTE_TIE), &cTrk);
                                    cTrk.pos += 2;
                                }
                            } else {
                                // repeat midi key
                                playNote(cTrk, Note(cTrk.lastNoteKey, cTrk.lastNoteVel, NOTE_TIE), &cTrk);
                                cTrk.pos += 1;
                            }
                            break;
                        default:
                            throw MyException(string("unsupported command (decimal): ") + to_string(cmd));
                    } // end main cmd switch
                } else {
                    int8_t len = cTrk.lastNoteLen = noteLut.at(cmd);
                    if (reader.PeekInt8(0) >= 0) {
                        // new midi key
                        if (reader.PeekInt8(1) >= 0) {
                            // new note velocity
                            if (reader.PeekInt8(2) >= 0) {
                                // add gate time
                                uint8_t key = cTrk.lastNoteKey = reader.ReadUInt8();
                                uint8_t vel = cTrk.lastNoteVel = reader.ReadUInt8();
                                len += reader.ReadInt8();
                                playNote(cTrk, Note(key, vel, len), &cTrk);
                                cTrk.pos += 4;
                            } else {
                                // no gate time
                                uint8_t key = cTrk.lastNoteKey = reader.ReadUInt8();
                                uint8_t vel = cTrk.lastNoteVel = reader.ReadUInt8();
                                playNote(cTrk, Note(key, vel, len), &cTrk);
                                cTrk.pos += 3;
                            }
                        } else {
                            // repeast note velocity
                            uint8_t key = cTrk.lastNoteKey = reader.ReadUInt8();
                            playNote(cTrk, Note(key, cTrk.lastNoteVel, len), &cTrk);
                            cTrk.pos += 2;
                        }
                    } else {
                        // repeat midi key
                        playNote(cTrk, Note(cTrk.lastNoteKey, cTrk.lastNoteVel, len), &cTrk);
                        cTrk.pos += 1;
                    }
                }
            } // end of processing loop
        } // end of single tick processing handler
        if (updatePV || cTrk.mod > 0) {
            sm.SetTrackPV((void *)&cTrk, 
                    cTrk.GetLeftVol(),
                    cTrk.GetRightVol(),
                    cTrk.GetPitch());
        }
        isFirst = false;
    } // end of track iteration
    if (!isSongRunning) {
        sm.FadeOut(SONG_FINISH_TIME);
        isEnding = true;
    }
} // end processSequenceTick

void StreamGenerator::playNote(Sequence::Track& trk, Note note, void *owner)
{
    if (trk.prog > 127)
        return;

    switch (sbnk.GetInstrType(trk.prog, note.midiKey)) {
        case InstrType::PCM:
            sm.NewSoundChannel(
                    owner, 
                    sbnk.GetSampInfo(trk.prog, note.midiKey),
                    sbnk.GetADSR(trk.prog, note.midiKey),
                    note,
                    trk.GetLeftVol(),
                    trk.GetRightVol(),
                    trk.GetPitch(),
                    false);
        case InstrType::PCM_FIXED:
            sm.NewSoundChannel(
                    owner, 
                    sbnk.GetSampInfo(trk.prog, note.midiKey),
                    sbnk.GetADSR(trk.prog, note.midiKey),
                    note,
                    trk.GetLeftVol(),
                    trk.GetRightVol(),
                    trk.GetPitch(),
                    true);
            break;
        case InstrType::SQ1:
            sm.NewCGBNote(
                    owner, 
                    sbnk.GetCGBDef(trk.prog, note.midiKey),
                    sbnk.GetADSR(trk.prog, note.midiKey),
                    note, 
                    trk.GetLeftVol(), 
                    trk.GetRightVol(), 
                    trk.GetPitch(), 
                    CGBType::SQ1);
            break;
        case InstrType::SQ2:
            sm.NewCGBNote(
                    owner, 
                    sbnk.GetCGBDef(trk.prog, note.midiKey),
                    sbnk.GetADSR(trk.prog, note.midiKey),
                    note, 
                    trk.GetLeftVol(), 
                    trk.GetRightVol(), 
                    trk.GetPitch(), 
                    CGBType::SQ2);
            break;
        case InstrType::WAVE:
            sm.NewCGBNote(
                    owner, 
                    sbnk.GetCGBDef(trk.prog, note.midiKey),
                    sbnk.GetADSR(trk.prog, note.midiKey),
                    note, 
                    trk.GetLeftVol(), 
                    trk.GetRightVol(), 
                    trk.GetPitch(), 
                    CGBType::WAVE);
            break;
        case InstrType::NOISE:
            sm.NewCGBNote(
                    owner, 
                    sbnk.GetCGBDef(trk.prog, note.midiKey),
                    sbnk.GetADSR(trk.prog, note.midiKey),
                    note, 
                    trk.GetLeftVol(), 
                    trk.GetRightVol(), 
                    trk.GetPitch(), 
                    CGBType::NOISE);
            break;
        case InstrType::INVALID:
            return;
    }
}
