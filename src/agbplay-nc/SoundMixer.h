#pragma once

#include <vector>
#include <list>
#include <cstdint>
#include <bitset>
#include <memory>

#include "ReverbEffect.h"
#include "SoundChannel.h"
#include "CGBChannel.h"
#include "Constants.h"

struct MP2KContext;

class SoundMixer
{
public:
    SoundMixer(MP2KContext& ctx, uint32_t sampleRate, float masterVolume);
    SoundMixer(const SoundMixer&) = delete;
    SoundMixer& operator=(const SoundMixer&) = delete;

    void Init(uint32_t fixedModeRate, uint8_t reverb, float pcmMasterVolume, ReverbType rtype);

    void Process();
    size_t GetSamplesPerBuffer() const;
    uint32_t GetSampleRate() const;
    void ResetFade();
    void StartFadeOut(float millis);
    void StartFadeIn(float millis);
    bool IsFadeDone() const;

private:
    MP2KContext& ctx;

    uint32_t sampleRate;
    uint32_t fixedModeRate = 13379;
    size_t samplesPerBuffer = sampleRate / (AGB_FPS * INTERFRAMES);

    // volume control related stuff

    float masterVolume;
    float pcmMasterVolume = masterVolume;
    float fadePos = 1.0f;
    float fadeStepPerMicroframe = 0.0f;
    size_t fadeMicroframesLeft = 0;
};
