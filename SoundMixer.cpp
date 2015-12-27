#include <algorithm>
#include <cmath>

#include "SoundMixer.h"
#include "MyException.h"

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
    this->att = 0xFF;
    this->dec = 0x00;
    this->sus = 0xFF;
    this->rel = 0x00;
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

SoundChannel::SoundChannel(void *owner, SampleInfo sInfo, ADSR env, Note note, uint8_t leftVol, uint8_t rightVol, int16_t pitch, bool fixed)
{
    this->owner = owner;
    this->note = note;
    this->env = env;
    this->sInfo = sInfo;
    this->interPos = 0.0f;
    SetVol(leftVol, rightVol);
    this->fixed = fixed;
    SetPitch(pitch);
    // if instant attack is ative directly max out the envelope to not cut off initial sound
    this->eState = EnvState::INIT;
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
    this->leftVol = note.velocity * leftVol / 128;
    this->rightVol = note.velocity * rightVol / 128;
}

uint8_t SoundChannel::GetVolL()
{
    return leftVol;
}

uint8_t SoundChannel::GetVolR()
{
    return rightVol;
}

bool SoundChannel::TickNote()
{
    if (eState < EnvState::REL) {
        if (note.length > 0) {
            note.length--;
            if (note.length == 0) {
                eState = EnvState::REL;
                return false;
            }
            return true;
        } else if (note.length == -1) {
            return true;
        } else throw MyException("ShoundChannel::NoteTick shouldn't be able to crash");
    } else {
        return false;
    }
}

bool SoundChannel::IsDead()
{
    return (eState == EnvState::DEAD) ? true : false;
}

void SoundChannel::SetPitch(int16_t pitch)
{
    freq = sInfo.midCfreq * powf(2.0f, float(note.midiKey - 60) / 12.0f + float(pitch) / 768.0f);
}

/*
 * public CGBChannel
 */

CGBChannel::CGBChannel(CGBType t)
{
    this->cType = t;
    this->interPos = 0.0f;
    this->owner = nullptr;
    this->leftVol = 0;
    this->rightVol = 0;
    this->eState = EnvState::SUS;
}

CGBChannel::~CGBChannel()
{
}

void CGBChannel::Init(void *owner, Note note, ADSR env)
{
    this->owner = owner;
    this->note = note;
    this->env = env;
    this->eState = EnvState::INIT;
}

float CGBChannel::GetFreq()
{
    return freq;
}

void CGBChannel::SetVol(uint8_t leftVol, uint8_t rightVol)
{
    // TODO
}

uint8_t CGBChannel::GetVolL()
{
    return leftVol;
}

uint8_t CGBChannel::GetVolR()
{
    return rightVol;
}

bool CGBChannel::TickNote()
{
    if (eState < EnvState::REL) {
        if (note.length > 0) {
            note.length--;
            if (note.length == 0) {
                eState = EnvState::REL;
                return false;
            }
            return true;
        } else if (note.length == -1) {
            return true;
        } else throw MyException("ShoundChannel::NoteTick shouldn't be able to crash");
    } else {
        return false;
    }
}

void CGBChannel::SetPitch(int16_t pitch)
{
    // TODO
}

/*
 * public SoundMixer
 */

SoundMixer::SoundMixer(uint32_t sampleRate, uint32_t fixedModeRate) : sq1(CGBType::SQ1), sq2(CGBType::SQ2), wave(CGBType::WAVE), noise(CGBType::NOISE)
{
    this->sampleRate = sampleRate;
    this->samplesPerBuffer = sampleRate / (AGB_FPS * INTERFRAMES);
    this->sampleBuffer = vector<float>(N_CHANNELS * samplesPerBuffer);
    fill_n(this->sampleBuffer.begin(), this->sampleBuffer.size(), 0.0f);
}

SoundMixer::~SoundMixer()
{
}

void SoundMixer::NewSoundChannel(void *owner, SampleInfo sInfo, ADSR env, Note note, uint8_t leftVol, uint8_t rightVol, int16_t pitch, bool fixed)
{
    sndChannels.emplace_back(owner, sInfo, env, note, leftVol, rightVol, pitch, fixed);
}

void SoundMixer::NewCGBNote(void *owner, ADSR env, Note note, uint8_t leftVol, uint8_t rightVol, int16_t pitch, uint8_t chn)
{
    CGBChannel& nChn = sq1;
    switch (chn) {
        case 1: nChn = sq1; break;
        case 2: nChn = sq2; break;
        case 3: nChn = wave; break;
        case 4: nChn = noise; break;
    }
    nChn.Init(owner, note, env);
    nChn.SetVol(leftVol, rightVol);
}

void SoundMixer::SetTrackPV(void *owner, uint8_t leftVol, uint8_t rightVol, int16_t pitch)
{
    for (SoundChannel& sc : sndChannels) {
        if (sc.GetOwner() == owner) {
            sc.SetVol(leftVol, rightVol);
            sc.SetPitch(pitch);
        }
    }
}

int SoundMixer::TickTrackNotes(void *owner)
{
    int active = 0;
    for (SoundChannel& chn : sndChannels) 
    {
        if (chn.GetOwner() == owner) {
            if (chn.TickNote())
                active++;
        }
    }
    return active;
}

void SoundMixer::StopChannel(void *owner, uint8_t key)
{
    // TODO
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

/*
 * private SoundMixer
 */

void SoundMixer::purgeChannels()
{
    sndChannels.remove_if([](SoundChannel& chn) { return chn.IsDead(); });
}
