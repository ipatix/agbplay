#include "StreamGenerator.h"
#include "MyException.h"

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

const std::map<uint8_t, int8_t> StreamGenerator::delayLut = {
    {0x81,1 }, {0x82,2 }, {0x83,3 }, {0x84,4 }, {0x85,5 }, {0x86,6 }, {0x87,7 }, {0x88,8 },
    {0x89,9 }, {0x8A,10}, {0x8B,11}, {0x8C,12}, {0x8D,13}, {0x8E,14}, {0x8F,15}, {0x90,16},
    {0x91,17}, {0x92,18}, {0x93,19}, {0x94,20}, {0x95,21}, {0x96,22}, {0x97,23}, {0x98,24},
    {0x99,28}, {0x9A,30}, {0x9B,32}, {0x9C,36}, {0x9D,40}, {0x9E,42}, {0x9F,44}, {0xA0,48},
    {0xA1,52}, {0xA2,52}, {0xA3,56}, {0xA4,60}, {0xA5,64}, {0xA6,66}, {0xA7,68}, {0xA8,72},
    {0xA9,76}, {0xAA,78}, {0xAB,80}, {0xAC,84}, {0xAD,88}, {0xAE,90}, {0xAF,92}, {0xB0,96}
};

const std::map<uint8_t, int8_t> StreamGenerator::noteLut = {
    {0xD0,1 }, {0xD1,2 }, {0xD2,3 }, {0xD3,4 }, {0xD4,5 }, {0xD5,6 }, {0xD6,7 }, {0xD7,8 },
    {0xD8,9 }, {0xD9,10}, {0xDA,11}, {0xDB,12}, {0xDC,13}, {0xDD,14}, {0xDE,15}, {0xDF,16},
    {0xE0,17}, {0xE1,18}, {0xE2,19}, {0xE3,20}, {0xE4,21}, {0xE5,22}, {0xE6,23}, {0xE7,24},
    {0xE8,28}, {0xE9,30}, {0xEA,32}, {0xEB,36}, {0xEC,40}, {0xED,42}, {0xEE,44}, {0xEF,48},
    {0xF0,52}, {0xF1,52}, {0xF2,56}, {0xF3,60}, {0xF4,64}, {0xF5,66}, {0xF6,68}, {0xF7,72},
    {0xF8,76}, {0xF9,78}, {0xFA,80}, {0xFB,84}, {0xFC,88}, {0xFD,90}, {0xFE,92}, {0xFF,96}
};



StreamGenerator::StreamGenerator(Sequence& seq, uint32_t outSampleRate, EnginePars ep) : seq(seq), sm(outSampleRate)
{
    this->ep = ep;
}

StreamGenerator::~StreamGenerator()
{
}

uint32_t StreamGenerator::GetBufferUnitCount()
{
    return sm.GetBufferUnitCount();
}

void *StreamGenerator::ProcessAndGetAudio()
{
    processSequenceFrame();
    return sm.ProcessAndGetAudio();;
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
    Rom& reader = seq.getRom();
    // process all tracks
    for (Sequence::Track& cTrk : seq.tracks) {
        if (!cTrk.isRunning)
            continue;

        reader.Seek(cTrk.pos);

        // count down last delay and process
        if (--cTrk.delay <= 0) {
            while (true) {
                uint8_t cmd = reader.ReadUInt8();
                if (cmd <= 0x7F) {
                    switch (cTrk.lastEvent) {
                        case LEvent::NONE:                                    break;
                        case LEvent::VOL:    cTrk.vol   = cmd;                break;
                        case LEvent::PAN:    cTrk.pan   = int8_t(cmd - 0x40); break;
                        case LEvent::BEND:   cTrk.bend  = int8_t(cmd - 0x40); break;
                        case LEvent::BENDR:  cTrk.bendr = cmd;                break;
                        case LEvent::MOD:    cTrk.mod   = cmd;                break;
                        case LEvent::TUNE:   cTrk.tune  = int8_t(cmd - 0x40); break;
                        case LEvent::NOTE:
                                          // TODO
                                          break;
                        case LEvent::TIE:
                                          // TODO
                                          break;
                        case LEvent::EOT:
                                          // TODO
                                          break;
                        default: break;
                    }
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
                            cTrk.pos = reader.ReadAGBPtrToPos();
                            break;
                        case 0xB3:
                            // PATT, call sub
                            if (cTrk.retStackPos >= MAX_TRK_CALL)
                                throw MyException("Too many nested Track calls");

                            cTrk.retStack[cTrk.retStackPos++] = cTrk.pos + 5;
                            cTrk.pos = reader.ReadAGBPtrToPos();
                            break;
                        case 0xB4:
                            // PEND, end of sub
                            if (cTrk.retStackPos == 0) {
                                // nothing is called so ignore it
                                cTrk.pos++;
                            } else {
                                cTrk.pos = cTrk.retStack[--cTrk.retStackPos];
                            }
                            break;
                        case 0xBB:
                            // TEMPO
                            seq.bpm = reader.ReadUInt8() * 2;
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
                            // TODO update note volumes
                            break;
                        case 0xBF:
                            // PAN
                            cTrk.lastEvent = LEvent::PAN;
                            cTrk.pan = int8_t(reader.ReadUInt8() - 0x40);
                            cTrk.pos += 2;
                            // TODO update note volumes
                            break;
                        case 0xC0:
                            // BEND
                            cTrk.lastEvent = LEvent::BEND;
                            cTrk.bend = int8_t(reader.ReadUInt8() - 0x40);
                            cTrk.pos += 2;
                            // update pitch
                            break;
                        case 0xC1:
                            // BENDR
                            cTrk.lastEvent = LEvent::BENDR;
                            cTrk.bendr = reader.ReadUInt8();
                            cTrk.pos += 2;
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
                            // FIXME maybe do conditional updates upon changing the modt
                            break;
                        case 0xC8:
                            // TUNE
                            cTrk.lastEvent = LEvent::TUNE;
                            cTrk.tune = int8_t(reader.ReadUInt8() - 128);
                            cTrk.pos += 2;
                            break;
                        case 0xCE:
                            // EOT
                            cTrk.lastEvent = LEvent::EOT;
                            // TODO implement
                            break;
                        case 0xCF:
                            // TIE
                            cTrk.lastEvent = LEvent::TIE;
                            // TODO implement
                            break;
                    }
                } else {
                    // TODO note processing
                }
            }
        }
    }
}
