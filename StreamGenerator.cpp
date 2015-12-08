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
