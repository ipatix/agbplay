#include <algorithm>
#include <cmath>
#include <cassert>

#include "SoundMixer.h"
#include "Xcept.h"
#include "Debug.h"
#include "Util.h"

using namespace std;
using namespace agbplay;

/*
 * public SoundMixer
 */

SoundMixer::SoundMixer(uint32_t sampleRate, uint32_t fixedModeRate, uint8_t reverb, float mvl, ReverbType rtype, uint8_t ntracks)
    : sq1(), sq2(), wave(), noise()
{
    samplesPerBuffer = sampleRate / (AGB_FPS * INTERFRAMES);
    for (size_t i = 0; i < ntracks; i++)
    {
        switch (rtype) {
            case ReverbType::NORMAL:
                revdsps.push_back(new ReverbEffect(reverb, sampleRate, uint8_t(0x630 / (fixedModeRate / AGB_FPS))));
                break;
            case ReverbType::GS1:
                revdsps.push_back(new ReverbGS1(reverb, sampleRate, uint8_t(0x630 / (fixedModeRate / AGB_FPS))));
                break;
            case ReverbType::GS2:
                revdsps.push_back(new ReverbGS2(reverb, sampleRate, uint8_t(0x630 / (fixedModeRate / AGB_FPS)),
                        0.4140625f, -0.0625f));
                break;
            case ReverbType::MGAT:
                revdsps.push_back(new ReverbGS2(reverb, sampleRate, uint8_t(0x630 / (fixedModeRate / AGB_FPS)),
                        0.25f, -0.046875f));
                break;
            case ReverbType::TEST:
                revdsps.push_back(new ReverbTest(reverb, sampleRate, uint8_t(0x630 / (fixedModeRate / AGB_FPS))));
                break;
            default:
                throw Xcept("Invalid Reverb Effect");
        }
        soundBuffers.emplace_back(N_CHANNELS * samplesPerBuffer);
        fill(soundBuffers[i].begin(), soundBuffers[i].end(), 0.0f);
    }
    activeBackBuffer.reset();
    this->sampleRate = sampleRate;
    this->fixedModeRate = fixedModeRate;
    sampleRateReciprocal = 1.0f / float(sampleRate);
    masterVolume = MASTER_VOL;
    pcmMasterVolume = MASTER_VOL * mvl;
    fadeMicroframesLeft = 0;
    fadePos = 1.0f;
    fadeStepPerMicroframe = 0.0f;
    this->ntracks = ntracks;
}

SoundMixer::~SoundMixer()
{
    while (!revdsps.empty())
    {
        delete revdsps.back();
        revdsps.pop_back();
    }
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
        default: throw Xcept("FATAL ERROR");
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
    if (key == NOTE_ALL)
        __print_debug("stopchannel ALL");
    for (SoundChannel& chn : sndChannels) 
    {
        if (chn.GetOwner() == owner && (
                    key == NOTE_ALL || (
                        chn.GetMidiKey() == key &&
                        chn.GetNoteLength() == NOTE_TIE))) {
            chn.Release();
            //return;
        }
    }
    if (sq1.GetOwner() == owner && (
                key == NOTE_ALL || (
                    sq1.GetMidiKey() == key && 
                    sq1.GetNoteLength() == NOTE_TIE))) {
        sq1.Release();
        //return;
    }
    if (sq2.GetOwner() == owner && (
                key == NOTE_ALL || (
                    sq2.GetMidiKey() == key &&
                    sq2.GetNoteLength() == NOTE_TIE))) {
        sq2.Release();
        //return;
    }
    if (wave.GetOwner() == owner && (
                key == NOTE_ALL || (
                    wave.GetMidiKey() == key &&
                    wave.GetNoteLength() == NOTE_TIE))) {
        wave.Release();
        //return;
    }
    if (noise.GetOwner() == owner && (
                key == NOTE_ALL || (
                    noise.GetMidiKey() == key &&
                    noise.GetNoteLength() == NOTE_TIE))) {
        noise.Release();
        //return;
    }
}

std::vector<std::vector<float>>& SoundMixer::ProcessAndGetAudio()
{
    clearBuffers();
    renderToBuffers();
    purgeChannels();
    return soundBuffers;
}

size_t SoundMixer::GetActiveChannelCount()
{
    return sndChannels.size();
}

size_t SoundMixer::GetBufferUnitCount()
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
    fadeMicroframesLeft = size_t(millis / 1000.0f * float(AGB_FPS * INTERFRAMES));
    fadeStepPerMicroframe = -1.0f / float(fadeMicroframesLeft);
}

void SoundMixer::FadeIn(float millis)
{
    fadePos = 0.0f;
    fadeMicroframesLeft = size_t(millis / 1000.0f * float(AGB_FPS * INTERFRAMES));
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

void SoundMixer::clearBuffers()
{
    for (vector<float>& b : soundBuffers)
    {
        fill(b.begin(), b.end(), 0.0f);
    }
}

void SoundMixer::renderToBuffers()
{
    // master volume calculation
    float masterFrom = masterVolume;
    float masterTo = masterVolume;
    if (fadeMicroframesLeft > 0) {
        if (fadePos < 0.f) {
            masterFrom = 0.f;
        } else {
            masterFrom *= powf(fadePos, 10.0f / 6.0f);
        }
        fadePos += fadeStepPerMicroframe;
        if (fadePos < 0.f) {
            masterTo = 0.f;
        } else {
            masterTo *= powf(fadePos, 10.0f / 6.0f);
        }
        fadeMicroframesLeft--;
    }

    MixingArgs margs;
    margs.vol = pcmMasterVolume;
    margs.fixedModeRate = fixedModeRate;
    margs.sampleRateReciprocal = sampleRateReciprocal;
    margs.nBlocksReciprocal = 1.0f / float(samplesPerBuffer);

    // process all digital channels
    for (SoundChannel& chn : sndChannels)
    {
        assert(chn.GetOwner() < soundBuffers.size());
        chn.Process(soundBuffers[chn.GetOwner()].data(), samplesPerBuffer, margs);
    }

    // apply PCM reverb

    assert(revdsps.size() == soundBuffers.size());
    for (size_t i = 0; i < soundBuffers.size(); i++)
    {
        revdsps[i]->ProcessData(soundBuffers[i].data(), samplesPerBuffer);
    }

    // process all CGB channels

    if (sq1.GetOwner() != INVALID_OWNER) {
        assert(sq1.GetOwner() <= soundBuffers.size());
        sq1.Process(soundBuffers[sq1.GetOwner()].data(), samplesPerBuffer, margs);
    }
    if (sq2.GetOwner() != INVALID_OWNER) {
        assert(sq2.GetOwner() <= soundBuffers.size());
        sq2.Process(soundBuffers[sq2.GetOwner()].data(), samplesPerBuffer, margs);
    }
    if (wave.GetOwner() != INVALID_OWNER) {
        assert(wave.GetOwner() <= soundBuffers.size());
        wave.Process(soundBuffers[wave.GetOwner()].data(), samplesPerBuffer, margs);
    }
    if (noise.GetOwner() != INVALID_OWNER) {
        assert(noise.GetOwner() <= soundBuffers.size());
        noise.Process(soundBuffers[noise.GetOwner()].data(), samplesPerBuffer, margs);
    }

    for (vector<float>& b : soundBuffers)
    {
        float masterStep = (masterTo - masterFrom) * margs.nBlocksReciprocal;
        float masterLevel = masterFrom;
        for (size_t i = 0; i < samplesPerBuffer; i++)
        {
            b[i * N_CHANNELS] *= masterLevel;
            b[i * N_CHANNELS + 1] *= masterLevel;

            masterLevel +=  masterStep;
        }
    }
}
