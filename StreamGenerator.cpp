#include "StreamGenetator.h"
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

EnginePars::EngiePars()
{
}

/*
 * public StreamGenerator
 */

StreamGenerator::StreamGenerator(Sequence& seq, EnginePars ep) : this->seq(seq), sm(48000)
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
    while (bpmStack >= 0) {
        processSequenceTick();
        seq.bpmStack -= seq.bpm * INTERFRAMES;
    }
    seq.bpmStack += BPM_PER_FRAME;
}

void StreamGenerator::processSequenceTick()
{
    Rom& reader = seq.getRom();
    // process all tracks
    for (Track& cTrk : seq.tracks) {
        if (!cTrk.isRunning)
            continue;

        reader.Seek(cTrk.pos);

        // count down last delay and process
        if (--cTrk.delay <= 0) {
            while (true) {
                uint8_t cmd = reader.ReadUInt8();
                if (cmd <= 0x7F) {
                    // repeat command TODO
                } else if (cmd == 0x80) {
                    // NOP delay
                    cTrk.pos++;
                    continue;
                } else if (cmd <= 0xB0) {
                    // normal delay
                    cTrk.pos++;
                    cTrk.delay = delayLuy[cmd];
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
                            cTrk.keyShift = reader.ReadUInt8();
                            cTrk.pos += 2;
                            break;
                        case 0xBD:
                            // VOICE
                            cTrk.prog = reader.ReadUInt8();
                            cTrk.pos += 2;
                            break;
                        case 0xBE:
                            // VOL
                            cTrk.vol = reader.ReadUInt8();
                            cTrl.pos += 2;
                            // TODO update note volumes
                            break;
                        case 0xBF:
                            // PAN
                            break;
                        case 0xC0:
                            // BEND
                            break;
                        case 0xC1:
                            // BENDR
                            break;
                        case 0xC2:
                            // LFOS
                            break;
                        case 0xC3:
                            // LFODL
                            break;
                        case 0xC4:
                            // MOD
                            break;
                        case 0xC5:
                            // MODT,
                            break;
                        case 0xC8:
                            // TUNE
                            break;
                        case 0xCE:
                            // EOT
                            break;
                        case 0xCF:
                            // TIE
                            break;
                    }
                } else {
                    // TODO note processing
                }
            }
        }
    }
}
