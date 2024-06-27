#include <algorithm>
#include <cmath>
#include <cassert>

#include "SoundMixer.h"
#include "Xcept.h"
#include "Util.h"
#include "MP2KContext.h"

/*
 * public SoundMixer
 */

SoundMixer::SoundMixer(MP2KContext& ctx, uint32_t sampleRate, float masterVolume)
    : ctx(ctx), sampleRate(sampleRate), masterVolume(masterVolume)
{
}

void SoundMixer::Init(uint32_t fixedModeRate, uint8_t reverb, float pcmMasterVolume, ReverbType rtype)
{
    // TODO with access to ctx, rtype is an unnecessary parameter
    this->fixedModeRate = fixedModeRate;
    this->samplesPerBuffer = sampleRate / (AGB_FPS * INTERFRAMES);
    this->pcmMasterVolume = pcmMasterVolume;

    const uint8_t numDmaBuffers = std::max(
        static_cast<uint8_t>(2),
        static_cast<uint8_t>(ctx.agbplaySoundMode.dmaBufferLen / (fixedModeRate / AGB_FPS))
    );

    // TODO add for-loop for multiple players
    for (MP2KPlayer &player : ctx.players) {
        for (MP2KTrack &trk : player.tracks) {
            trk.reverb = ReverbEffect::MakeReverb(rtype, reverb, sampleRate, numDmaBuffers);
        }
    }
}

void SoundMixer::Process()
{
    /* 1. clear the mixing buffer before processing channels */
    ctx.masterAudioBuffer.resize(samplesPerBuffer);
    std::fill(ctx.masterAudioBuffer.begin(), ctx.masterAudioBuffer.end(), sample{0.0f, 0.0f});

    for (MP2KPlayer &player : ctx.players) {
        for (MP2KTrack &trk : player.tracks) {
            trk.audioBuffer.resize(samplesPerBuffer);
            std::fill(trk.audioBuffer.begin(), trk.audioBuffer.end(), sample{0.0f, 0.0f});
        }
    }

    /* 2. prepare arguments for mixing */
    MixingArgs margs;
    margs.vol = pcmMasterVolume;
    margs.fixedModeRate = fixedModeRate;
    margs.sampleRateInv = 1.0f / static_cast<float>(sampleRate);
    margs.samplesPerBufferInv= 1.0f / static_cast<float>(samplesPerBuffer);
    margs.curInterFrame = ctx.GetCurInterFrame();

    /* 3. mix channels which are affected by reverb (PCM only) */
    auto mixFunc = [&](auto &channels) {
        for (auto &chn : channels)
            chn.Process(chn.trackOrg->audioBuffer.data(), samplesPerBuffer, margs);
    };
    mixFunc(ctx.sndChannels);

    /* 4. apply reverb */
    // TODO add player for-loop for multiple players
    for (MP2KPlayer &player : ctx.players) {
        for (MP2KTrack &trk : player.tracks) {
            trk.reverb->ProcessData(trk.audioBuffer.data(), samplesPerBuffer);
        }
    }

    /* 5. mix channels which are not affected by reverb (CGB) */
    mixFunc(ctx.sq1Channels);
    mixFunc(ctx.sq2Channels);
    mixFunc(ctx.waveChannels);
    mixFunc(ctx.noiseChannels);

    /* 6. clean up all stopped channels */
    auto removeFunc = [](const auto& chn) { return chn.envState == EnvState::DEAD; };
    ctx.sndChannels.remove_if(removeFunc);
    ctx.sq1Channels.remove_if(removeFunc);
    ctx.sq2Channels.remove_if(removeFunc);
    ctx.waveChannels.remove_if(removeFunc);
    ctx.noiseChannels.remove_if(removeFunc);

    /* 7. apply fadeout if active */
    // TODO move this to FadeOutMain
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

    for (MP2KPlayer &player : ctx.players) {
        for (MP2KTrack &trk : player.tracks) {
            const float masterStep = (masterTo - masterFrom) * margs.samplesPerBufferInv;
            float masterLevel = masterFrom;
            for (size_t i = 0; i < samplesPerBuffer; i++)
            {
                trk.audioBuffer[i].left *= masterLevel;
                trk.audioBuffer[i].right *= masterLevel;

                masterLevel +=  masterStep;
            }
        }
    }

    /* 8. master mixdown */
    for (MP2KPlayer &player : ctx.players) {
        for (MP2KTrack &trk : player.tracks) {
            if (trk.muted)
                continue;

            assert(ctx.masterAudioBuffer.size() == trk.audioBuffer.size());
            for (size_t i = 0; i < ctx.masterAudioBuffer.size(); i++) {
                ctx.masterAudioBuffer[i].left += trk.audioBuffer[i].left;
                ctx.masterAudioBuffer[i].right += trk.audioBuffer[i].right;
            }
        }
    }
}

size_t SoundMixer::GetSamplesPerBuffer() const
{
    return samplesPerBuffer;
}

uint32_t SoundMixer::GetSampleRate() const
{
    return sampleRate;
}

void SoundMixer::ResetFade()
{
    fadePos = 0.0f;
    fadeMicroframesLeft = 0;
}

void SoundMixer::StartFadeOut(float millis)
{
    fadePos = 1.0f;
    fadeMicroframesLeft = size_t(millis / 1000.0f * float(AGB_FPS * INTERFRAMES));
    fadeStepPerMicroframe = -1.0f / float(fadeMicroframesLeft);
}

void SoundMixer::StartFadeIn(float millis)
{
    fadePos = 0.0f;
    fadeMicroframesLeft = size_t(millis / 1000.0f * float(AGB_FPS * INTERFRAMES));
    fadeStepPerMicroframe = 1.0f / float(fadeMicroframesLeft);
}

bool SoundMixer::IsFadeDone() const
{
    return fadeMicroframesLeft == 0;
}