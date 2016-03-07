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
    : sq1(), sq2(), wave(), noise()
{
    this->activeBackBuffer.reset();
    this->revdsp = new ReverbEffect(reverb, sampleRate, uint8_t(0x630 / (fixedModeRate / AGB_FPS)));
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
}

SoundMixer::~SoundMixer()
{
    delete revdsp;
}

void SoundMixer::NewSoundChannel(void *owner, SampleInfo sInfo, ADSR env, Note note, uint8_t vol, int8_t pan, int16_t pitch, bool fixed)
{
    sndChannels.emplace_back(owner, sInfo, env, note, vol, pan, pitch, fixed);
}

void SoundMixer::NewCGBNote(void *owner, CGBDef def, ADSR env, Note note, uint8_t vol, int8_t pan, int16_t pitch, CGBType type)
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
    nChn->Init(owner, def, note, env);
    nChn->SetVol(vol, pan);
    nChn->SetPitch(pitch);
}

void SoundMixer::SetTrackPV(void *owner, uint8_t vol, int8_t pan, int16_t pitch)
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

int SoundMixer::TickTrackNotes(void *owner, bitset<NUM_NOTES>& activeNotes)
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

void SoundMixer::StopChannel(void *owner, uint8_t key)
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
    float pcmMasterFrom = pcmMasterVolume;
    float pcmMasterTo = pcmMasterVolume;
    if (this->fadeMicroframesLeft > 0) {
        masterFrom *= powf(this->fadePos, 10.0f / 6.0f);
        pcmMasterFrom *= powf(this->fadePos, 10.0f / 6.0f);
        fadePos += this->fadeStepPerMicroframe;
        masterTo *= powf(this->fadePos, 10.0f / 6.0f);
        pcmMasterTo *= powf(this->fadePos, 10.0f / 6.0f);
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
        vol.fromVolLeft *= pcmMasterFrom;
        vol.fromVolRight *= pcmMasterFrom;
        vol.toVolLeft *= pcmMasterTo;
        vol.toVolRight *= pcmMasterTo;
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
            interStep *= (1.0f / 64.0f); // gs instruments use a different step scale
            // switch by GS type
            if (uSamplePtr[1] == 0) {
                // pulse wave
                #define DUTY_BASE 2
                #define DUTY_STEP 3
                #define DEPTH 4
                #define INIT_DUTY 5
                uint32_t fromPos;
                uint8_t step = chn.GetInterStep();
                if (step == 0) {
                    fromPos = chn.pos += uint32_t(uSamplePtr[DUTY_STEP] << 24);
                } else {
                    fromPos = chn.pos;
                }
                uint32_t toPos = fromPos + uint32_t(uSamplePtr[DUTY_STEP] << 24);

                auto calcThresh = [](uint32_t val, uint8_t base, uint8_t depth, uint8_t init) {
                    uint32_t iThreshold = uint32_t(init << 24) + val;
                    iThreshold = int32_t(iThreshold) < 0 ? ~iThreshold >> 8 : iThreshold >> 8;
                    iThreshold = iThreshold * depth + uint32_t(base << 24);
                    return float(iThreshold >> 16) * (1.0f / 65536.0f);
                };

                float fromThresh = calcThresh(fromPos, uSamplePtr[DUTY_BASE], uSamplePtr[DEPTH], uSamplePtr[INIT_DUTY]);
                float toThresh = calcThresh(toPos, uSamplePtr[DUTY_BASE], uSamplePtr[DEPTH], uSamplePtr[INIT_DUTY]);

                float deltaThresh = toThresh - fromThresh;
                float baseThresh = fromThresh + (deltaThresh * (float(step) * (1.0f / float(INTERFRAMES))));
                float threshStep = deltaThresh * (1.0f / float(INTERFRAMES)) * nBlocksReciprocal;
                float fThreshold = baseThresh;
                #undef DUTY_BASE
                #undef DUTY_STEP
                #undef DEPTH
                #undef INIT_DUTY

                for (uint32_t cnt = nBlocks; cnt > 0; cnt--)
                {
                    float baseSamp = chn.interPos < fThreshold ? 0.5f : -0.5f;
                    // correct dc offset
                    baseSamp += 0.5f - fThreshold;
                    fThreshold += threshStep;
                    *buf++ += baseSamp * lVol;
                    *buf++ += baseSamp * rVol;

                    lVol += lVolDeltaStep;
                    rVol += rVolDeltaStep;

                    chn.interPos += interStep;
                    // this below might glitch for too high frequencies, which usually shouldn't be used anyway
                    if (chn.interPos >= 1.0f) chn.interPos -= 1.0f;
                }

            } else if (uSamplePtr[1] == 1) {
                // saw wave
                const uint32_t fix = 0x70;

                for (uint32_t cnt = nBlocks; cnt > 0; cnt--)
                {
                    /*
                     * Sorry that the baseSamp calculation looks ugly.
                     * For accuracy it's a 1 to 1 translation of the original assembly code
                     */
                    chn.interPos += interStep;
                    if (chn.interPos >= 1.0f) chn.interPos -= 1.0f;
                    uint32_t var1 = uint32_t(chn.interPos * 256) - fix;
                    uint32_t var2 = uint32_t(chn.interPos * 65536.0f) << 17;
                    uint32_t var3 = var1 - (var2 >> 27);
                    chn.pos = var3 + uint32_t(int32_t(chn.pos) >> 1);

                    float baseSamp = float((int32_t)chn.pos) / 256.0f;

                    *buf++ += baseSamp * lVol;
                    *buf++ += baseSamp * rVol;

                    lVol += lVolDeltaStep;
                    rVol += rVolDeltaStep;
                }
            } else {
                // triangluar shaped wave
                for (uint32_t cnt = nBlocks; cnt > 0; cnt--)
                {
                    chn.interPos += interStep;
                    if (chn.interPos >= 1.0f) chn.interPos -= 1.0f;
                    float baseSamp;
                    if (chn.interPos < 0.5f) {
                        baseSamp = (4.0f * chn.interPos) - 1.0f;
                    } else {
                        baseSamp = 3.0f - (4.0f * chn.interPos);
                    }

                    *buf++ += baseSamp * lVol;
                    *buf++ += baseSamp * rVol;

                    lVol += lVolDeltaStep;
                    rVol += rVolDeltaStep;
                }
            }
        } else {
            for (uint32_t cnt = nBlocks; cnt > 0; cnt--)
            {
                float baseSamp = float(info.samplePtr[chn.pos]) / 128.0f;
                float deltaSamp = float(info.samplePtr[chn.pos+1]) / 128.0f - baseSamp;
                float finalSamp = baseSamp + deltaSamp * chn.interPos;
                // ugh, cosine interpolation sounds worse, disabled
                /*float samp1 = float(info.samplePtr[chn.pos]) / 128.0f;
                float samp2 = float(info.samplePtr[chn.pos + 1]) / 128.0f;
                float amp = (samp1 - samp2) * (1.0f / 2.0f);
                float avg = (samp1 + samp2) * (1.0f / 2.0f);
                float finalSamp = amp * cosf(chn.interPos * (float)M_PI) + avg;*/

                *buf++ += finalSamp * lVol;
                *buf++ += finalSamp * rVol;

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

    // apply PCM reverb

    revdsp->ProcessData(sampleBuffer.data(), samplesPerBuffer);

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

    sq1.StepEnvelope();
    if (sq1.GetState() != EnvState::DEAD) {
        // square 1

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
    }
    sq1.UpdateVolFade();

    // square 2

    sq2.StepEnvelope();
    if (sq2.GetState() != EnvState::DEAD) {
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
    }
    sq2.UpdateVolFade();

    // wave

    wave.StepEnvelope();
    if (wave.GetState() != EnvState::DEAD) {
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
            wave.pos = (wave.pos + posDelta) & 0x1F;
        }
    }
    wave.UpdateVolFade();

    // noise

    noise.StepEnvelope();
    if (noise.GetState() != EnvState::DEAD) {
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
    noise.UpdateVolFade();
}
