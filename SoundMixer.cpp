#include <algorithm>
#include <cmath>

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
 * public SampleInfo
 */

SampleInfo::SampleInfo(int8_t *samplePtr, float midCfreq, bool loopEnabled, uint32_t loopPos, uint32_t endPos)
{
    this->samplePtr = samplePtr;
    this->midCfreq = midCfreq;
    this->loopPos = loopPos;
    this->endPos = endPos;
    this->loopEnabled = loopEnabled;
}

SampleInfo::SampleInfo()
{
}

/*
 * public SoundChannel
 */

SoundChannel::SoundChannel(void *owner, SampleInfo sInfo, ADSR env, Note note, uint8_t leftVol, uint8_t rightVol, int16_t pitch)
{
    this->owner = owner;
    this->note = note;
    this->env = env;
    this->sInfo = sInfo;
    SetVol(leftVol, rightVol);
    this->processLeftVol = note.velocity * leftVol / 128;
    this->processRightVol = note.velocity * rightVol / 128;
    SetPitch(pitch);
    // if instant attack is ative directly max out the envelope to not cut off initial sound
    if (env.att == 0xFF) {
        this->processEnvelope = 0xFF;
    } else {
        this->processEnvelope = 0;
    }
}

SoundChannel::~SoundChannel()
{
}

void *SoundChannel::GetOwner()
{
    return owner;
}

float SoundChannel::GetFreq()
{
    return freq;
}

void SoundChannel::SetVol(uint8_t leftVol, uint8_t rightVol)
{
    this->leftVol = uint8_t(note.velocity * leftVol / 128);
    this->rightVol = uint8_t(note.velocity * rightVol / 128);
}

uint8_t SoundChannel::GetVolL()
{
    return leftVol;
}

uint8_t SoundChannel::GetVolR()
{
    return rightVol;
}

void SoundChannel::SetPitch(int16_t pitch)
{
    freq = sInfo.midCfreq * powf(2.0f, float(note.midiKey - 60) / 12.0f + float(pitch) / 768.0f);
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
    this->sampleBuffer = vector<float>(N_CHANNELS * samplesPerBuffer);
    fill_n(this->sampleBuffer.begin(), this->sampleBuffer.size(), 0.0f);
}

SoundMixer::~SoundMixer()
{
}

void SoundMixer::NewChannel(void *owner, SampleInfo sInfo, ADSR env, Note note, uint8_t leftVol, uint8_t rightVol, int16_t pitch)
{
    sndChannels.emplace_back(owner, sInfo, env, note, leftVol, rightVol, pitch);
}

void SoundMixer::SetAllTrackPars(void *owner, uint8_t leftVol, uint8_t rightVol, int16_t pitch)
{
    for (SoundChannel& sc : sndChannels) {
        if (sc.GetOwner() == owner) {
            sc.SetVol(leftVol, rightVol);
            sc.SetPitch(pitch);
        }
    }
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
