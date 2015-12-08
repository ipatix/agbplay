#include "SoundMixer.h"

using namespace std;
using namespace agbplay;

/*
 * public SoundChannel
 */

SoundMixer::SoundMixer(uint32_t sampleRate)
{
    this->sampleRate = sampleRate;
    this->samplesPerBuffer = sampleRate / (AGB_FPS * INTERFRAMES);
}

SoundMixer::~SoundMixer()
{
}


