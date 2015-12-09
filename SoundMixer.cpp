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
    this->sampleBuffer = std::vector<float>(N_CHANNELS * samplesPerBuffer);
}

SoundMixer::~SoundMixer()
{
}

void *SoundMixer::ProcessAndGetAudio()
{
    // TODO implement
    return nullptr;
}

uint32_t SoundMixer::GetBufferUnitCount()
{
    return sampelsPerBuffer;
}
