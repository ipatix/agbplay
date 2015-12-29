#include <algorithm>
#include <cmath>
#include <cassert>

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

uint8_t SoundChannel::GetMidiKey()
{
    return note.midiKey;
}

void SoundChannel::Release()
{
    if (eState < EnvState::REL)
        eState = EnvState::REL;
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
        } else assert(false);
    } else {
        return false;
    }
}

EnvState SoundChannel::GetState()
{
    return eState;
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

void CGBChannel::Init(void *owner, CGBDef def, Note note, ADSR env)
{
    this->owner = owner;
    this->note = note;
    this->def = def;
    this->env = env;
    this->eState = EnvState::INIT;
}

void *CGBChannel::GetOwner()
{
    return owner;
}

float CGBChannel::GetFreq()
{
    return freq;
}

void CGBChannel::SetVol(uint8_t leftVol, uint8_t rightVol)
{
#ifdef CGB_PAN_SNAP
    if (leftVol << 1 > rightVol) {
        // snap left
        this->leftVol = (leftVol + rightVol) >> 5;
        this->rightVol = 0;
    } else if (rightVol << 1 > leftVol) {
        // snap right
        this->rightVol= (leftVol + rightVol) >> 5;
        this->leftVol = 0;
    } else {
        // snap mid
        this->leftVol = this->rightVol = (leftVol + rightVol) >> 5;
    }
#else
    this->leftVol = leftVol >> 4;
    this->rightVol = rightVol >> 4;
#endif
}

uint8_t CGBChannel::GetVolL()
{
    return leftVol;
}

uint8_t CGBChannel::GetVolR()
{
    return rightVol;
}

uint8_t CGBChannel::GetMidiKey()
{
    return note.midiKey;
}

void CGBChannel::Release()
{
    if (eState < EnvState::REL)
        eState = EnvState::REL;
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

EnvState CGBChannel::GetState()
{
    return eState;
}

void CGBChannel::SetPitch(int16_t pitch)
{
    freq = powf(2.0f, float(note.midiKey - 60) / 12.0f + float(pitch) / 768.0f);
}

/*
 * public SoundMixer
 */

SoundMixer::SoundMixer(uint32_t sampleRate, uint32_t fixedModeRate) : sq1(CGBType::SQ1), sq2(CGBType::SQ2), wave(CGBType::WAVE), noise(CGBType::NOISE)
{
    this->sampleRate = sampleRate;
    this->samplesPerBuffer = sampleRate / (AGB_FPS * INTERFRAMES);
    this->sampleBuffer = vector<float>(N_CHANNELS * samplesPerBuffer);
    this->fixedModeRate = fixedModeRate;
    fill_n(this->sampleBuffer.begin(), this->sampleBuffer.size(), 0.0f);
    this->isShuttingDown = false;
}

SoundMixer::~SoundMixer()
{
}

void SoundMixer::NewSoundChannel(void *owner, SampleInfo sInfo, ADSR env, Note note, uint8_t leftVol, uint8_t rightVol, int16_t pitch, bool fixed)
{
    sndChannels.emplace_back(owner, sInfo, env, note, leftVol, rightVol, pitch, fixed);
}

void SoundMixer::NewCGBNote(void *owner, CGBDef def, ADSR env, Note note, uint8_t leftVol, uint8_t rightVol, int16_t pitch, CGBType type)
{
    CGBChannel *nChn;
    switch (type) {
        case CGBType::SQ1: nChn = &sq1; break;
        case CGBType::SQ2: nChn = &sq2; break;
        case CGBType::WAVE: nChn = &wave; break;
        case CGBType::NOISE: nChn = &noise; break;
    }
    nChn->Init(owner, def, note, env);
    nChn->SetVol(leftVol, rightVol);
    nChn->SetPitch(pitch);
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
    for (SoundChannel& chn : sndChannels) 
    {
        if (chn.GetOwner() == owner && chn.GetMidiKey() == key && chn.GetState() < EnvState::REL) {
            chn.Release();
            return;
        }
    }
    if (sq1.GetOwner() == owner && sq1.GetMidiKey() == key && sq1.GetState() < EnvState::REL) {
        sq1.Release();
        return;
    }
    if (sq2.GetOwner() == owner && sq2.GetMidiKey() == key && sq2.GetState() < EnvState::REL) {
        sq2.Release();
        return;
    }
    if (wave.GetOwner() == owner && wave.GetMidiKey() == key && wave.GetState() < EnvState::REL) {
        wave.Release();
        return;
    }
    if (noise.GetOwner() == owner && noise.GetMidiKey() == key && noise.GetState() < EnvState::REL) {
        noise.Release();
        return;
    }
}

void *SoundMixer::ProcessAndGetAudio()
{
    // TODO implement
    if (isShuttingDown)
        return nullptr;
    else {
        clearBuffer();
        return (void *)&sampleBuffer[0];
    }
}

uint32_t SoundMixer::GetBufferUnitCount()
{
    return samplesPerBuffer;
}

void SoundMixer::Shutdown()
{
    this->isShuttingDown = true;
    for (SoundChannel& chn : sndChannels)
    {
        chn.Release();
    }
    sq1.Release();
    sq2.Release();
    wave.Release();
    noise.Release();
}

/*
 * private SoundMixer
 */

void SoundMixer::purgeChannels()
{
    sndChannels.remove_if([](SoundChannel& chn) { return chn.GetState() == EnvState::DEAD; });
}

void SoundMixer::clearBuffer()
{
    fill(sampleBuffer.begin(), sampleBuffer.end(), 0.0f);
}
