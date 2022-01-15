#include <algorithm>
#include <cmath>
#include <cassert>

#include "SoundMixer.h"
#include "Xcept.h"
#include "Debug.h"
#include "Util.h"
#include "ConfigManager.h"
#include "PlayerContext.h"

/*
 * public SoundMixer
 */

SoundMixer::SoundMixer(PlayerContext& ctx, uint32_t sampleRate, float masterVolume)
    : ctx(ctx), sampleRate(sampleRate), masterVolume(masterVolume)
{
}

void SoundMixer::Init(uint32_t fixedModeRate, uint8_t reverb, float pcmMasterVolume, ReverbType rtype, uint8_t numTracks)
{
    this->fixedModeRate = fixedModeRate;
    this->samplesPerBuffer = sampleRate / (AGB_FPS * INTERFRAMES);
    this->numTracks = numTracks;
    this->pcmMasterVolume = pcmMasterVolume;

    GameConfig& gameCfg = ConfigManager::Instance().GetCfg();

    revdsps.resize(numTracks);
    for (size_t i = 0; i < numTracks; i++)
    {
        switch (rtype) {
        case ReverbType::NORMAL:
            revdsps[i] = std::make_unique<ReverbEffect>(
                    reverb, sampleRate, uint8_t(gameCfg.GetRevBufSize() / (fixedModeRate / AGB_FPS)));
            break;
        case ReverbType::NONE:
            revdsps[i] = std::make_unique<ReverbEffect>(
                    0, sampleRate, uint8_t(gameCfg.GetRevBufSize() / (fixedModeRate / AGB_FPS)));
            break;
        case ReverbType::GS1:
            revdsps[i] = std::make_unique<ReverbGS1>(
                    reverb, sampleRate, uint8_t(gameCfg.GetRevBufSize() / (fixedModeRate / AGB_FPS)));
            break;
        case ReverbType::GS2:
            revdsps[i] = std::make_unique<ReverbGS2>(
                    reverb, sampleRate, uint8_t(gameCfg.GetRevBufSize() / (fixedModeRate / AGB_FPS)),
                    0.4140625f, -0.0625f);
            break;
            // Mario Power Tennis uses same coefficients as Mario Golf Advance Tour
        case ReverbType::MGAT:
            revdsps[i] = std::make_unique<ReverbGS2>(
                    reverb, sampleRate, uint8_t(gameCfg.GetRevBufSize() / (fixedModeRate / AGB_FPS)),
                    0.25f, -0.046875f);
            break;
        case ReverbType::TEST:
            revdsps[i] = std::make_unique<ReverbTest>(
                    reverb, sampleRate, uint8_t(gameCfg.GetRevBufSize() / (fixedModeRate / AGB_FPS)));
            break;
        default:
            throw Xcept("Invalid Reverb Effect");
        }
    }
}

void SoundMixer::Process(std::vector<std::vector<sample>>& outputBuffers)
{
    /* 1. match number of output buffers to the number of tracks we have */
    if (outputBuffers.size() != numTracks) {
        outputBuffers.resize(numTracks, std::vector<sample>(samplesPerBuffer));
    }

    /* 2. clear the mixing buffer before processing channels */
    for (auto& outputBuffer : outputBuffers) {
        assert(outputBuffer.size() == samplesPerBuffer);
        std::fill(outputBuffer.begin(), outputBuffer.end(), sample{0.0f, 0.0f});
    }

    /* 3. prepare arguments for mixing */
    MixingArgs margs;
    margs.vol = pcmMasterVolume;
    margs.fixedModeRate = fixedModeRate;
    margs.sampleRateInv = 1.0f / static_cast<float>(sampleRate);
    margs.samplesPerBufferInv= 1.0f / static_cast<float>(samplesPerBuffer);

    /* 4. mix channels which are affected by reverb (PCM only) */
    for (auto& chn : ctx.sndChannels) {
        assert(chn.GetTrackIdx() < numTracks);
        chn.Process(outputBuffers[chn.GetTrackIdx()].data(), samplesPerBuffer, margs);
    }

    /* 5. apply reverb */
    assert(revdsps.size() == numTracks);
    for (size_t i = 0; i < outputBuffers.size(); i++)
        revdsps[i]->ProcessData(outputBuffers[i].data(), samplesPerBuffer);

    /* 6. mix channels which are not affected by reverb (CGB) */
    for (auto& chn : ctx.sq1Channels) {
        assert(chn.GetTrackIdx() < numTracks);
        chn.Process(outputBuffers[chn.GetTrackIdx()].data(), samplesPerBuffer, margs);
    }
    for (auto& chn : ctx.sq2Channels) {
        assert(chn.GetTrackIdx() < numTracks);
        chn.Process(outputBuffers[chn.GetTrackIdx()].data(), samplesPerBuffer, margs);
    }
    for (auto& chn : ctx.waveChannels) {
        assert(chn.GetTrackIdx() < numTracks);
        chn.Process(outputBuffers[chn.GetTrackIdx()].data(), samplesPerBuffer, margs);
    }
    for (auto& chn : ctx.noiseChannels) {
        assert(chn.GetTrackIdx() < numTracks);
        chn.Process(outputBuffers[chn.GetTrackIdx()].data(), samplesPerBuffer, margs);
    }

    /* 7. clean up all stopped channels */
    ctx.sndChannels.remove_if([](const auto& chn) { return chn.GetState() == EnvState::DEAD; });
    ctx.sq1Channels.remove_if([](const auto& chn) { return chn.GetState() == EnvState::DEAD; });
    ctx.sq2Channels.remove_if([](const auto& chn) { return chn.GetState() == EnvState::DEAD; });
    ctx.waveChannels.remove_if([](const auto& chn) { return chn.GetState() == EnvState::DEAD; });
    ctx.noiseChannels.remove_if([](const auto& chn) { return chn.GetState() == EnvState::DEAD; });

    /* 8. apply fadeout if active */
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

    for (auto& outputBuffer : outputBuffers) {
        float masterStep = (masterTo - masterFrom) * margs.samplesPerBufferInv;
        float masterLevel = masterFrom;
        for (size_t i = 0; i < samplesPerBuffer; i++)
        {
            outputBuffer[i].left *= masterLevel;
            outputBuffer[i].right *= masterLevel;

            masterLevel +=  masterStep;
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
