#include "StreamGenetator.h"

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
    for (int i = 0; i < seq.getNumTrks(); i++) {
        Track& cTrk = seq.getTrk(i);
        reader.Seek(cTrk.pos);
        // count down last delay and process
        if (--cTrk.delay <= 0) {
            while (true) {
                uint8_t cmd = reader.ReadUInt8();
                // TODO continue here 
            }
        }
    }
}
