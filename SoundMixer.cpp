#include "SoundMixer.h"

using namespace std;
using namespace agbplay;

/*
 * ADSR
 */

ADSR::ADSR(uint8_t att, uint8_t dec, uint8_t sus, uint8_t rel)
{
    this->att = att;
    this->dec = dec;
    this->sus = sus;
    this->rel = rel;
}

ADSR::ADSR()
{
}

/*
 * Note
 */

Note::Note(uint8_t midiKey, uint8_t velocity, int8_t length)
{
    this->midiKey = midiKey;
    this->velocity = velocity;
    this->length = length;
}

Note::Note()
{
}

/*
 * public SoundChannel
 */

SoundChannel::SoundChannel(int8_t *samplePtr, ADSR env, Note note)
{
    this->samplePtr = samplePtr;
    this->note = note;
    this->env = env;
}

SoundChannel::~SoundChannel()
{
}

/*
 * public CGBChannel
 */

CGBChannel::CGBChannel()
{
}

CGBChannel::~CGBChannel()
{
}

/*
 * public SoundMixer
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
    return samplesPerBuffer;
}
