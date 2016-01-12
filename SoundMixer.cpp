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

SoundMixer::SoundMixer(uint32_t sampleRate, uint32_t fixedModeRate, int reverb, float mvl) 
: sq1(CGBType::SQ1), sq2(CGBType::SQ2), wave(CGBType::WAVE), noise(CGBType::NOISE)
{
    this->revdsp = new ReverbEffect(reverb, sampleRate, uint8_t(0x630 / (fixedModeRate / AGB_FPS)));
    this->sampleRate = sampleRate;
    this->samplesPerBuffer = sampleRate / (AGB_FPS * INTERFRAMES);
    this->sampleBuffer = vector<float>(N_CHANNELS * samplesPerBuffer);
    this->fixedModeRate = fixedModeRate;
    fill_n(this->sampleBuffer.begin(), this->sampleBuffer.size(), 0.0f);
    this->sampleRateReciprocal = 1.0f / float(sampleRate);
    this->masterVolume = MASTER_VOL * mvl;
    this->fadeMicroframesLeft = 0;
    this->fadePos = 1.0f;
    this->fadeStepPerMicroframe = 0.0f;
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
    CGBChannel& nChn = sq1;
    switch (type) {
        case CGBType::SQ1: nChn = sq1; break;
        case CGBType::SQ2: nChn = sq2; break;
        case CGBType::WAVE: nChn = wave; break;
        case CGBType::NOISE: nChn = noise; break;
    }
    nChn.Init(owner, def, note, env);
    nChn.SetVol(leftVol, rightVol);
    nChn.SetPitch(pitch);
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
    if (sq1.GetOwner() == owner && sq1.TickNote())
        active++;
    if (sq2.GetOwner() == owner && sq2.TickNote())
        active++;
    if (wave.GetOwner() == owner && wave.TickNote())
        active++;
    if (noise.GetOwner() == owner && noise.TickNote())
        active++;
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
    uint32_t nBlocks = uint32_t(sampleBuffer.size() / N_CHANNELS);
    float nBlocksReciprocal = 1.0f / float(nBlocks);

    // process all digital channels
    for (SoundChannel& chn : sndChannels)
    {
        chn.StepEnvelope();
        if (chn.GetState() == EnvState::DEAD)
            continue;

        ChnVol vol = chn.GetVol();
        vol.fromVolLeft *= masterFrom;
        vol.fromVolRight *= masterFrom;
        vol.toVolLeft *= masterTo;
        vol.toVolRight *= masterTo;
        SampleInfo& info = chn.GetInfo();
        float lVolDeltaStep = (vol.toVolLeft - vol.fromVolLeft) * nBlocksReciprocal;
        float rVolDeltaStep = (vol.toVolRight - vol.fromVolRight) * nBlocksReciprocal;
        float lVol = vol.fromVolLeft;
        float rVol = vol.fromVolRight;
        float interStep;
        if (chn.IsFixed()) {
            interStep = float(this->fixedModeRate) * this->sampleRateReciprocal;
        } else {
            interStep = chn.GetFreq() * this->sampleRateReciprocal;
        }
        float *buf = &sampleBuffer[0];
        if (chn.IsGS()) {
            uint8_t *uSamplePtr = (uint8_t *)info.samplePtr;
            // switch by GS type
            if (uSamplePtr[1] == 0) {
                // pulse wave
                if ((chn.pos & 0x3) == 0) {
                    chn.pos += uint32_t(uSamplePtr[3] << 24);
                    chn.pos |= 0x3;
                } else {
                    chn.pos -= 1;
                }
                uint32_t iThreshold = uint32_t(uSamplePtr[5] << 24) + chn.pos;
                iThreshold = int32_t(iThreshold) < 0 ? uint32_t(int32_t(iThreshold) * -1) >> 8 : iThreshold >> 8;
                iThreshold = iThreshold * uSamplePtr[4] + uint32_t(uSamplePtr[2] << 24);
                float fThreshold = float(iThreshold >> 16) / 65536.0f;
                interStep *= 0.015625f;
                for (uint32_t cnt = nBlocks; cnt > 0; cnt--)
                {
                    float baseSamp = chn.interPos < fThreshold ? 0.5f : -0.5f;
                    *buf++ += baseSamp * lVol;
                    *buf++ += baseSamp * rVol;

                    lVol += lVolDeltaStep;
                    rVol += rVolDeltaStep;

                    chn.interPos += interStep;
                    if (chn.interPos > 1.0f) chn.interPos -= 1.0f;
                }
            } else if (uSamplePtr[1] == 1) {
                // traingluar shaped wave
            } else {
                // saw wave
            }
        } else {
            //__print_debug("Processing Channel");
            //__print_debug(("Vol: " + to_string(lVol) + ", " + to_string(rVol)).c_str());
            for (uint32_t cnt = nBlocks; cnt > 0; cnt--)
            {
                float baseSamp = float(info.samplePtr[chn.pos]) / 128;
                float deltaSamp = float(info.samplePtr[chn.pos+1]) / 128 - baseSamp;
                float finalSamp = baseSamp + deltaSamp * chn.interPos;

                *buf++ += finalSamp * lVol;
                //__print_debug(FormatString("Rend Smp %p val %f", buf-1, *(buf-1)));
                *buf++ += finalSamp * rVol;
                //__print_debug(FormatString("Rend Smp %p val %f", buf-1, *(buf-1)));

                lVol += lVolDeltaStep;
                rVol += rVolDeltaStep;

                chn.interPos += interStep;
                uint32_t posDelta = uint32_t(chn.interPos);
                chn.interPos -= float(posDelta);
                chn.pos += posDelta;
                if (chn.pos >= info.endPos) {
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

    sq1.StepEnvelope();
    vol = sq1.GetVol();
    vol.fromVolLeft *= masterFrom;
    vol.fromVolRight *= masterFrom;
    vol.toVolLeft *= masterTo;
    vol.toVolRight *= masterTo;
    info = sq1.GetDef();
    pat = sq1.GetPat();
    assert(pat);
    lVolDeltaStep = (vol.toVolLeft - vol.fromVolLeft) * nBlocksReciprocal;
    rVolDeltaStep = (vol.toVolRight - vol.fromVolRight) * nBlocksReciprocal;
    lVol = vol.fromVolLeft;
    rVol = vol.fromVolRight;
    interStep = sq1.GetFreq() * this->sampleRateReciprocal;
    buf = &sampleBuffer[0];

    // TODO add sweep functionality
    for (uint32_t cnt = nBlocks; cnt > 0; cnt--)
    {
        float samp = pat[sq1.pos];

        *buf++ += samp * lVol;
        *buf++ += samp * rVol;

        lVol += lVolDeltaStep;
        rVol += rVolDeltaStep;

        sq1.interPos += interStep;
        uint32_t posDelta = uint32_t(sq1.interPos);
        sq1.interPos -= float(posDelta);
        sq1.pos = (sq1.pos + posDelta) & 0x7;
    }

    // square 2

    sq2.StepEnvelope();
    vol = sq2.GetVol();
    vol.fromVolLeft *= masterFrom;
    vol.fromVolRight *= masterFrom;
    vol.toVolLeft *= masterTo;
    vol.toVolRight *= masterTo;
    info = sq2.GetDef();
    pat = sq2.GetPat();
    assert(pat);
    lVolDeltaStep = (vol.toVolLeft - vol.fromVolLeft) * nBlocksReciprocal;
    rVolDeltaStep = (vol.toVolRight - vol.fromVolRight) * nBlocksReciprocal;
    lVol = vol.fromVolLeft;
    rVol = vol.fromVolRight;
    interStep = sq2.GetFreq() * this->sampleRateReciprocal;
    buf = &sampleBuffer[0];

    for (uint32_t cnt = nBlocks; cnt > 0; cnt--)
    {
        float samp = pat[sq2.pos];

        *buf++ += samp * lVol;
        *buf++ += samp * rVol;

        lVol += lVolDeltaStep;
        rVol += rVolDeltaStep;

        sq2.interPos += interStep;
        uint32_t posDelta = uint32_t(sq2.interPos);
        sq2.interPos -= float(posDelta);
        sq2.pos = (sq2.pos + posDelta) & 0x7;
    }

    // wave

    wave.StepEnvelope();
    vol = wave.GetVol();
    vol.fromVolLeft *= masterFrom;
    vol.fromVolRight *= masterFrom;
    vol.toVolLeft *= masterTo;
    vol.toVolRight *= masterTo;
    info = wave.GetDef();
    pat = wave.GetPat();
    assert(pat);
    lVolDeltaStep = (vol.toVolLeft - vol.fromVolLeft) * nBlocksReciprocal;
    rVolDeltaStep = (vol.toVolRight - vol.fromVolRight) * nBlocksReciprocal;
    lVol = vol.fromVolLeft;
    rVol = vol.fromVolRight;
    interStep = wave.GetFreq() * this->sampleRateReciprocal;
    buf = &sampleBuffer[0];

    for (uint32_t cnt = nBlocks; cnt > 0; cnt--)
    {
        float samp = pat[wave.pos];

        *buf++ += samp * lVol;
        *buf++ += samp * rVol;

        lVol += lVolDeltaStep;
        rVol += rVolDeltaStep;

        wave.interPos += interStep;
        uint32_t posDelta = uint32_t(wave.interPos);
        wave.interPos -= float(posDelta);
        wave.pos = (wave.pos + posDelta) & 0xF;
    }
    //for (float& f : sampleBuffer)
    //    __print_debug(FormatString("PR Smp %p val %f", &f, f));

    // noise

    noise.StepEnvelope();
    vol = noise.GetVol();
    vol.fromVolLeft *= masterFrom;
    vol.fromVolRight *= masterFrom;
    vol.toVolLeft *= masterTo;
    vol.toVolRight *= masterTo;
    info = noise.GetDef();
    pat = noise.GetPat();
    assert(pat);
    lVolDeltaStep = (vol.toVolLeft - vol.fromVolLeft) * nBlocksReciprocal;
    rVolDeltaStep = (vol.toVolRight - vol.fromVolRight) * nBlocksReciprocal;
    lVol = vol.fromVolLeft;
    rVol = vol.fromVolRight;
    interStep = noise.GetFreq() * this->sampleRateReciprocal;
    buf = &sampleBuffer[0];

    uint32_t noise_bitmask = (info.np == NoisePatt::FINE) ? 0x7FFF : 0x7F;

    for (uint32_t cnt = nBlocks; cnt > 0; cnt--)
    {
        float samp = pat[noise.pos];

        *buf++ += samp * lVol;
        *buf++ += samp * rVol;

        lVol += lVolDeltaStep;
        rVol += rVolDeltaStep;

        noise.interPos += interStep;
        uint32_t posDelta = uint32_t(noise.interPos);
        noise.interPos -= float(posDelta);
        noise.pos = (noise.pos + posDelta) & noise_bitmask;
    }
}
