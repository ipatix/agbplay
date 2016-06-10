#include <algorithm>
#include <cmath>
#include <cassert>

#include "SoundMixer.h"
#include "MyException.h"
#include "Debug.h"
#include "Util.h"

using namespace std;
using namespace agbplay;

/*
 * public SoundMixer
 */

SoundMixer::SoundMixer(uint32_t sampleRate, uint32_t fixedModeRate, uint8_t reverb, float mvl, ReverbType rtype, uint8_t ntracks)
    : sq1(), sq2(), wave(), noise(), revdsp(rtype, reverb, sampleRate, uint8_t(0x630 / (fixedModeRate / AGB_FPS)))
{
    this->activeBackBuffer.reset();
    this->sampleRate = sampleRate;
    this->samplesPerBuffer = sampleRate / (AGB_FPS * INTERFRAMES);
    this->sampleBuffer = vector<float>(N_CHANNELS * samplesPerBuffer);
    this->fixedModeRate = fixedModeRate;
    fill_n(this->sampleBuffer.begin(), this->sampleBuffer.size(), 0.0f);
    this->sampleRateReciprocal = 1.0f / float(sampleRate);
    this->masterVolume = MASTER_VOL;
    this->pcmMasterVolume = MASTER_VOL * mvl;
    this->fadeMicroframesLeft = 0;
    this->fadePos = 1.0f;
    this->fadeStepPerMicroframe = 0.0f;
    this->ntracks = ntracks;
}

SoundMixer::~SoundMixer()
{
}

void SoundMixer::NewSoundChannel(uint8_t owner, SampleInfo sInfo, ADSR env, Note note, uint8_t vol, int8_t pan, int16_t pitch, bool fixed)
{
    sndChannels.emplace_back(owner, sInfo, env, note, vol, pan, pitch, fixed);
}

void SoundMixer::NewCGBNote(uint8_t owner, CGBDef def, ADSR env, Note note, uint8_t vol, int8_t pan, int16_t pitch, CGBType type)
{
    CGBChannel *nChn = &sq1;
    switch (type) {
        case CGBType::SQ1:
            nChn = &sq1; 
            break;
        case CGBType::SQ2: 
            nChn = &sq2; 
            break;
        case CGBType::WAVE: 
            nChn = &wave; 
            break;
        case CGBType::NOISE: 
            nChn = &noise; 
            break;
        default: throw MyException("FATAL ERROR");
    }
    if (nChn->GetState() < EnvState::REL && nChn->GetOwner() < owner)
        return;
    nChn->Init(owner, def, note, env);
    nChn->SetVol(vol, pan);
    nChn->SetPitch(pitch);
}

void SoundMixer::SetTrackPV(uint8_t owner, uint8_t vol, int8_t pan, int16_t pitch)
{
    for (SoundChannel& sc : sndChannels) {
        if (sc.GetOwner() == owner) {
            sc.SetVol(vol, pan);
            sc.SetPitch(pitch);
        }
    }
    if (sq1.GetOwner() == owner) {
        sq1.SetVol(vol, pan);
        sq1.SetPitch(pitch);
    }
    if (sq2.GetOwner() == owner) {
        sq2.SetVol(vol, pan);
        sq2.SetPitch(pitch);
    }
    if (wave.GetOwner() == owner) {
        wave.SetVol(vol, pan);
        wave.SetPitch(pitch);
    }
    if (noise.GetOwner() == owner) {
        noise.SetVol(vol, pan);
        noise.SetPitch(pitch);
    }
}

int SoundMixer::TickTrackNotes(uint8_t owner, bitset<NUM_NOTES>& activeNotes)
{
    activeBackBuffer.reset();
    int active = 0;
    for (SoundChannel& chn : sndChannels) 
    {
        if (chn.GetOwner() == owner) {
            if (chn.TickNote()) {
                active++;
                activeBackBuffer[chn.GetMidiKey() & 0x7F] = true;
            }
        }
    }
    if (sq1.GetOwner() == owner && sq1.TickNote()) {
        active++;
        activeBackBuffer[sq1.GetMidiKey() & 0x7F] = true;
    }
    if (sq2.GetOwner() == owner && sq2.TickNote()) {
        active++;
        activeBackBuffer[sq2.GetMidiKey() & 0x7F] = true;
    }
    if (wave.GetOwner() == owner && wave.TickNote()) {
        active++;
        activeBackBuffer[wave.GetMidiKey() & 0x7F] = true;
    }
    if (noise.GetOwner() == owner && noise.TickNote()) {
        active++;
        activeBackBuffer[noise.GetMidiKey() & 0x7F] = true;
    }
    // hopefully assignment goes well
    // otherwise we'll probably need to copy bit by bit
    activeNotes = activeBackBuffer;
    return active;
}

void SoundMixer::StopChannel(uint8_t owner, uint8_t key)
{
    for (SoundChannel& chn : sndChannels) 
    {
        if (chn.GetOwner() == owner && chn.GetMidiKey() == key && chn.GetState() < EnvState::REL && chn.GetNoteLength() == NOTE_TIE) {
            chn.Release();
            //return;
        }
    }
    if (sq1.GetOwner() == owner && sq1.GetMidiKey() == key && sq1.GetState() < EnvState::REL && sq1.GetNoteLength() == NOTE_TIE) {
        sq1.Release();
        //return;
    }
    if (sq2.GetOwner() == owner && sq2.GetMidiKey() == key && sq2.GetState() < EnvState::REL && sq2.GetNoteLength() == NOTE_TIE) {
        sq2.Release();
        //return;
    }
    if (wave.GetOwner() == owner && wave.GetMidiKey() == key && wave.GetState() < EnvState::REL && wave.GetNoteLength() == NOTE_TIE) {
        wave.Release();
        //return;
    }
    if (noise.GetOwner() == owner && noise.GetMidiKey() == key && noise.GetState() < EnvState::REL && noise.GetNoteLength() == NOTE_TIE) {
        noise.Release();
        //return;
    }
}

float *SoundMixer::ProcessAndGetAudio()
{
    clearBuffer();
    renderToBuffer();
    purgeChannels();
    return &sampleBuffer[0];
}

uint32_t SoundMixer::GetBufferUnitCount()
{
    return samplesPerBuffer;
}

uint32_t SoundMixer::GetRenderSampleRate()
{
    return sampleRate;
}

void SoundMixer::FadeOut(float millis)
{
    fadePos = 1.0f;
    fadeMicroframesLeft = uint32_t(millis / 1000.0f * float(AGB_FPS * INTERFRAMES));
    fadeStepPerMicroframe = -1.0f / float(fadeMicroframesLeft);
}

void SoundMixer::FadeIn(float millis)
{
    fadePos = 0.0f;
    fadeMicroframesLeft = uint32_t(millis / 1000.0f * float(AGB_FPS * INTERFRAMES));
    fadeStepPerMicroframe = 1.0f / float(fadeMicroframesLeft);
}

bool SoundMixer::IsFadeDone()
{
    return fadeMicroframesLeft == 0;
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

void SoundMixer::renderToBuffer()
{
    // master volume calculation
    float masterFrom = masterVolume;
    float masterTo = masterVolume;
    if (this->fadeMicroframesLeft > 0) {
        masterFrom *= powf(this->fadePos, 10.0f / 6.0f);
        fadePos += this->fadeStepPerMicroframe;
        masterTo *= powf(this->fadePos, 10.0f / 6.0f);
        this->fadeMicroframesLeft--;
    }

    MixingArgs margs;
    margs.vol = pcmMasterVolume;
    margs.fixedModeRate = fixedModeRate;
    margs.sampleRateReciprocal = sampleRateReciprocal;
    margs.nBlocksReciprocal = 1.0f / float(samplesPerBuffer);

    // process all digital channels
    for (SoundChannel& chn : sndChannels)
    {
        chn.Process(sampleBuffer.data(), samplesPerBuffer, margs);
    }

    // apply PCM reverb

    revdsp.ProcessData(sampleBuffer.data(), samplesPerBuffer);

    // process all CGB channels

    sq1.Process(sampleBuffer.data(), samplesPerBuffer, margs);
    sq2.Process(sampleBuffer.data(), samplesPerBuffer, margs);
    wave.Process(sampleBuffer.data(), samplesPerBuffer, margs);
    noise.Process(sampleBuffer.data(), samplesPerBuffer, margs);

    float masterStep = (masterTo - masterFrom) * margs.nBlocksReciprocal;
    float masterLevel = masterFrom;
    
    for (size_t i = 0; i < samplesPerBuffer; i++)
    {
        sampleBuffer[i * N_CHANNELS] *= masterLevel;
        sampleBuffer[i * N_CHANNELS + 1] *= masterLevel;

        masterLevel +=  masterStep;
    }
}
