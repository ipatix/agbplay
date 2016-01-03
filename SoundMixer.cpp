#include <algorithm>
#include <cmath>
#include <cassert>

#include "SoundMixer.h"
#include "MyException.h"

using namespace std;
using namespace agbplay;

/*
 * public SoundMixer
 */

SoundMixer::SoundMixer(uint32_t sampleRate, uint32_t fixedModeRate, int reverb) : sq1(CGBType::SQ1), sq2(CGBType::SQ2), wave(CGBType::WAVE), noise(CGBType::NOISE)
{
    this->revdsp = new ReverbEffect(reverb, sampleRate, uint8_t(0x630 / (fixedModeRate / AGB_FPS)));
    this->sampleRate = sampleRate;
    this->samplesPerBuffer = sampleRate / (AGB_FPS * INTERFRAMES);
    this->sampleBuffer = vector<float>(N_CHANNELS * samplesPerBuffer);
    this->fixedModeRate = fixedModeRate;
    fill_n(this->sampleBuffer.begin(), this->sampleBuffer.size(), 0.0f);
    this->isShuttingDown = false;
    this->sampleRateReciprocal = 1.0f / float(sampleRate);
}

SoundMixer::~SoundMixer()
{
    delete revdsp;
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
    if (isShuttingDown) {
        return nullptr;
    } else {
        clearBuffer();
        renderToBuffer();
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

void SoundMixer::renderToBuffer()
{
    uint32_t nBlocks = uint32_t(sampleBuffer.size());
    float nBlocksReciprocal = 1.0f / float(nBlocks);

    // process all digital channels
    for (SoundChannel& chn : sndChannels)
    {
        if (chn.GetState() == EnvState::DEAD)
            continue;

        ChnVol vol = chn.GetVol();
        SampleInfo& info = chn.GetInfo();
        float lVolDeltaStep = (vol.toVolLeft - vol.toVolLeft) * nBlocksReciprocal;
        float rVolDeltaStep = (vol.toVolRight - vol.toVolRight) * nBlocksReciprocal;
        float lVol = vol.fromVolLeft;
        float rVol = vol.fromVolRight;
        float interStep = chn.GetFreq() * this->sampleRateReciprocal;
        float *buf = &sampleBuffer[0];
        if (chn.IsGS()) {
            for (uint32_t cnt = nBlocks; cnt > 0; cnt--)
            {
                // TODO
            }
        } else {
            for (uint32_t cnt = nBlocks; cnt > 0; cnt--)
            {
                float baseSamp = float(info.samplePtr[chn.pos]) / 128;
                float deltaSamp = float(info.samplePtr[chn.pos+1]) / 128 - baseSamp;
                float finalSamp = baseSamp + deltaSamp * chn.interPos;

                *buf++ = finalSamp * lVol;
                *buf++ = finalSamp * rVol;

                lVol += lVolDeltaStep;
                rVol += rVolDeltaStep;

                chn.interPos += interStep;
                uint32_t posDelta = uint32_t(chn.interPos);
                chn.interPos -= float(posDelta);
                chn.pos += posDelta;
                if (chn.pos > info.endPos) {
                    if (info.loopEnabled) {
                        chn.pos = info.loopPos;
                    } else {
                        chn.Kill();
                        break;
                    }
                }
            }
        }
        chn.UpdateVolFade();
    }

    // process all CGB channels

    ChnVol vol;
    CGBDef info;
    float lVolDeltaStep;
    float rVolDeltaStep;
    float lVol;
    float rVol;
    float interStep;
    float *buf = nullptr;
    const float *pat = nullptr;

    // square 1

    vol = sq1.GetVol();
    info = sq1.GetDef();
    pat = sq1.GetPat();
    assert(pat);
    lVolDeltaStep = (vol.toVolLeft - vol.toVolLeft) * nBlocksReciprocal;
    rVolDeltaStep = (vol.toVolRight - vol.toVolRight) * nBlocksReciprocal;
    lVol = vol.fromVolLeft;
    rVol = vol.fromVolRight;
    interStep = sq1.GetFreq() * this->sampleRateReciprocal;
    buf = &sampleBuffer[0];

    // TODO add sweep functionality
    for (uint32_t cnt = nBlocks; cnt > 0; cnt--)
    {
        float samp = pat[sq1.pos];

        *buf++ = samp * lVol;
        *buf++ = samp * rVol;

        lVol += lVolDeltaStep;
        rVol += rVolDeltaStep;

        sq1.interPos += interStep;
        uint32_t posDelta = uint32_t(sq1.interPos);
        sq1.interPos -= float(posDelta);
        sq1.pos = (sq1.pos + posDelta) & 0x7;
    }

    // square 2

    vol = sq2.GetVol();
    info = sq2.GetDef();
    pat = sq2.GetPat();
    assert(pat);
    lVolDeltaStep = (vol.toVolLeft - vol.toVolLeft) * nBlocksReciprocal;
    rVolDeltaStep = (vol.toVolRight - vol.toVolRight) * nBlocksReciprocal;
    lVol = vol.fromVolLeft;
    rVol = vol.fromVolRight;
    interStep = sq2.GetFreq() * this->sampleRateReciprocal;
    buf = &sampleBuffer[0];

    for (uint32_t cnt = nBlocks; cnt > 0; cnt--)
    {
        float samp = pat[sq2.pos];

        *buf++ = samp * lVol;
        *buf++ = samp * rVol;

        lVol += lVolDeltaStep;
        rVol += rVolDeltaStep;

        sq2.interPos += interStep;
        uint32_t posDelta = uint32_t(sq2.interPos);
        sq2.interPos -= float(posDelta);
        sq2.pos = (sq2.pos + posDelta) & 0x7;
    }

    // wave

    vol = wave.GetVol();
    info = wave.GetDef();
    pat = wave.GetPat();
    assert(pat);
    lVolDeltaStep = (vol.toVolLeft - vol.toVolLeft) * nBlocksReciprocal;
    rVolDeltaStep = (vol.toVolRight - vol.toVolRight) * nBlocksReciprocal;
    lVol = vol.fromVolLeft;
    rVol = vol.fromVolRight;
    interStep = wave.GetFreq() * this->sampleRateReciprocal;
    buf = &sampleBuffer[0];

    for (uint32_t cnt = nBlocks; cnt > 0; cnt--)
    {
        float samp = pat[wave.pos];

        *buf++ = samp * lVol;
        *buf++ = samp * rVol;

        lVol += lVolDeltaStep;
        rVol += rVolDeltaStep;

        wave.interPos += interStep;
        uint32_t posDelta = uint32_t(wave.interPos);
        wave.interPos -= float(posDelta);
        wave.pos = (wave.pos + posDelta) & 0xF;
    }

    // noise

    vol = noise.GetVol();
    info = noise.GetDef();
    pat = noise.GetPat();
    assert(pat);
    lVolDeltaStep = (vol.toVolLeft - vol.toVolLeft) * nBlocksReciprocal;
    rVolDeltaStep = (vol.toVolRight - vol.toVolRight) * nBlocksReciprocal;
    lVol = vol.fromVolLeft;
    rVol = vol.fromVolRight;
    interStep = noise.GetFreq() * this->sampleRateReciprocal;
    buf = &sampleBuffer[0];

    uint32_t noise_bitmask = (info.np == NoisePatt::FINE) ? 0x7FFF : 0x7F;

    for (uint32_t cnt = nBlocks; cnt > 0; cnt--)
    {
        float samp = pat[noise.pos];

        *buf++ = samp * lVol;
        *buf++ = samp * rVol;

        lVol += lVolDeltaStep;
        rVol += rVolDeltaStep;

        noise.interPos += interStep;
        uint32_t posDelta = uint32_t(noise.interPos);
        noise.interPos -= float(posDelta);
        noise.pos = (noise.pos + posDelta) & noise_bitmask;
    }
}
