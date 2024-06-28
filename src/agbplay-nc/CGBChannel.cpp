#include <array>
#include <cmath>
#include <cassert>
#include <algorithm>

#include "CGBChannel.h"
#include "CGBPatterns.h"
#include "Xcept.h"
#include "Debug.h"
#include "Util.h"
#include "Constants.h"
#include "MP2KContext.h"

/*
 * public CGBChannel
 */

CGBChannel::CGBChannel(const MP2KContext &ctx, MP2KTrack *track, ADSR env, Note note, bool useStairstep)
    : MP2KChn(track, note, env), ctx(ctx), useStairstep(useStairstep)
{
    this->env.att &= 0x7;
    this->env.dec &= 0x7;
    this->env.sus &= 0xF;
    this->env.rel &= 0x7;

    //if (note.trackIdx == 6)
    //    Debug::print("note start: this=%p att=%d dec=%d sus=%d rel=%d", this, (int)env.att, (int)env.dec, (int)env.sus, (int)env.rel);
}

void CGBChannel::SetVol(uint16_t vol, int16_t pan)
{
    if (stop)
        return;

    /* CGB volume and pan aren't applied immediately due to the nature of being heavily
     * intertwined with the envelope handling. So save them and actually update them during envelope handling */
    this->vol = vol;
    /* Due to aggressive rounding apply pan "perfectly" like for PCM channels,
     * so clamp to 127 instead of 128 */
    this->pan = std::clamp<int16_t>(pan, -128, 127);
    this->mp2k_sus_vol_bug_update = true;
}

VolumeFade CGBChannel::getVol() const
{
    return volFade;
}

uint8_t CGBChannel::getPseudoEchoLevel() const
{
    return static_cast<uint8_t>(((envPeak * note.pseudoEchoVol) + 0xFF) >> 8);
}

float CGBChannel::timer2freq(float timer)
{
    assert(timer >= 0.0f);
    assert(timer <= 2047.0f);
    return 131072.0f / static_cast<float>(2048.0f - timer);
}

float CGBChannel::freq2timer(float freq)
{
    assert(freq > 0.0f);
    return 2048.0f - std::min(131072.0f / freq, 2047.0f);
}

void CGBChannel::Release() noexcept
{
    Release(false);
}

void CGBChannel::Release(bool fastRelease) noexcept
{
    this->stop = true;
    this->fastRelease = fastRelease;

    //if (note.trackIdx == 6)
    //    Debug::print("releasing: %d", fastRelease);
}

bool CGBChannel::TickNote() noexcept
{
    if (envState < EnvState::REL) {
        if (note.length > 0) {
            note.length--;
            if (note.length == 0) {
                /* Notes that stop on their own never release fast */
                Release(false);
                return false;
            }
        }
        return true;
    } else {
        return false;
    }
}

bool CGBChannel::IsFastReleasing() const
{
    return fastRelease;
}

bool CGBChannel::IsChn3() const
{
    return false;
}

void CGBChannel::stepEnvelope()
{
    if (envState == EnvState::INIT) {
        if (stop) {
            envState = EnvState::DEAD;
            return;
        }

        applyVol();
        panPrev = panCur;

        envInterStep = 0;

        envLevelCur = 0;
        envFrameCount = env.att;
        envState = EnvState::ATK;

        if (envFrameCount > 0) {
            envFadeLevel = 0.0f;
            return;
        } else {
            if (env.dec > 0) {
                envFadeLevel = envPeak;
            } else if (envSustain > 0) {
                envFadeLevel = envSustain;
            } else if (getPseudoEchoLevel() > 0) {
                envFadeLevel = getPseudoEchoLevel();
            }
            goto decay_start;
        }
    } else {
        if (fastRelease && envState != EnvState::DIE) {
            /* if note is already releasing but we are now releasing fast, do not wait
             * until the next real frame but stop the note immerdiately */
            if (env.rel == 0 || envState == EnvState::PSEUDO_ECHO)
                envInterStep = INTERFRAMES - 1;
            else
                envInterStep = 0;
            envState = EnvState::DIE;
            envFrameCount = 1;
            return;
        }

        if (++envInterStep < INTERFRAMES)
            return;

        envInterStep = 0;

        assert(envFrameCount > 0);
        envFrameCount--;
    }

    /* if fastRelease == true then stop == true */
    assert(!fastRelease || stop);

    if (envState == EnvState::PSEUDO_ECHO) {
        assert(note.pseudoEchoLen != 0);
        envFrameCount = 1;
        if (--note.pseudoEchoLen == 0) {
            envState = EnvState::DIE;
            envInterStep = INTERFRAMES - 1;
        }
    } else if (stop && envState < EnvState::REL) {
        envState = EnvState::REL;
        envFrameCount = env.rel;
        if (envLevelCur == 0 || envFrameCount == 0) {
            /* original doesn't branch on envLevelCur == 0, but we do in order to avoid
             * decreasing into the negative */
            goto pseudo_echo_start;
        } else {
            return;
        }
    } else if (envFrameCount == 0) {
        applyVol();

        if (envState == EnvState::REL) {
            assert(envLevelCur > 0);
            envLevelCur--;
            assert((int8_t)envLevelCur >= 0);

            if (envLevelCur == 0) {
pseudo_echo_start:
                envFrameCount = 1;
                envLevelCur = getPseudoEchoLevel();
                if (envLevelCur != 0 && note.pseudoEchoLen != 0) {
                    /* original doesn't check for pseudoEchoLen */
                    envState = EnvState::PSEUDO_ECHO;
                } else {
                    envState = EnvState::DIE;
                    envInterStep = INTERFRAMES - 1;
                    return;
                }
            } else {
                envFrameCount = env.rel;
                assert(env.rel != 0);
            }
        } else if (envState == EnvState::SUS) {
sustain_state:
            if (ctx.agbplaySoundMode.emulateCgbSustainBug) {
                envFrameCount = 7;
                if (IsChn3())
                    envLevelCur = envSustain;
                // else:
                //   envLevelCur is updated conditionally in applyVol() when when the sustain bug simulation is enabled
            } else {
                envFrameCount = 1;
                envLevelCur = envSustain;
            }
        } else if (envState == EnvState::DEC) {
            envLevelCur--;
            assert((int8_t)envLevelCur >= 0);

            if (envLevelCur <= envSustain) {
sustain_start:
                if (env.sus == 0) {
                    envState = EnvState::REL;
                    goto pseudo_echo_start;
                } else {
                    envState = EnvState::SUS;
                    envLevelCur = envSustain;
                    goto sustain_state;
                }
            }
            envFrameCount = env.dec;
            assert(env.dec != 0);
        } else if (envState == EnvState::ATK) {
            envLevelCur++;

            if (envLevelCur >= envPeak) {
decay_start:
                envState = EnvState::DEC;
                envFrameCount = env.dec;
                if (envPeak == 0 || envFrameCount == 0 || envPeak == envSustain) {
                    /* original code doesn't branch on envPeak == 0, but we do that we don't do a decrease envelope even
                     * though the level is already zero. Original also doesn't branch when peak == sustain, which we to do avoid
                     * an envelope decrease that will increase right after again. */
                    goto sustain_start;
                } else {
                    envLevelCur = envPeak;
                }
            } else {
                envFrameCount = env.att;
                assert(env.att != 0);
            }
        } else if (envState == EnvState::DIE) {
            envState = EnvState::DEAD;
            return;
        }

        assert(envFrameCount != 0);
    }
}

void CGBChannel::updateVolFade()
{
    size_t fadeInterframesCount = envFrameCount * INTERFRAMES - envInterStep;
    assert(fadeInterframesCount != 0);

    uint8_t envFadeLevelTo = 0xFF;
    switch (envState) {
    case EnvState::INIT:
        assert(false);
        break;
    case EnvState::ATK:
        assert(envLevelCur < 15);
        envFadeLevelTo = envLevelCur + 1;
        break;
    case EnvState::DEC:
    case EnvState::REL:
        assert(envLevelCur > 0);
        envFadeLevelTo = envLevelCur - 1;
        break;
    case EnvState::SUS:
    case EnvState::PSEUDO_ECHO:
        envFadeLevelTo = envLevelCur;
        fadeInterframesCount = 1;
        break;
    case EnvState::DIE:
        envFadeLevelTo = 0;
        break;
    case EnvState::DEAD:
        assert(false);
        break;
    }
    assert(envFadeLevelTo != 0xFF);

    float envFadeLevelNew;
    if (useStairstep) {
        /* In stairstep mode, the envelope will change in the last interframe before the next envelope update.
         * This fixes wave channels to behave like on hardware. */
        if (fadeInterframesCount == 1)
            envFadeLevelNew = envFadeLevelTo;
        else
            envFadeLevelNew = envFadeLevel;
    } else {
        envFadeLevelNew = envFadeLevel + (envFadeLevelTo - envFadeLevel) / static_cast<float>(fadeInterframesCount);
    }

    volFade.fromVolLeft = (panPrev == Pan::RIGHT) ? 0.0f : envFadeLevel * (1.0f / 32.0f);
    volFade.fromVolRight = (panPrev == Pan::LEFT) ? 0.0f : envFadeLevel * (1.0f / 32.0f);
    volFade.toVolLeft = (panCur == Pan::RIGHT) ? 0.0f : envFadeLevelNew * (1.0f / 32.0f);
    volFade.toVolRight = (panCur == Pan::LEFT) ? 0.0f : envFadeLevelNew * (1.0f / 32.0f);

    //if (note.trackIdx == 6)
    //    Debug::print("this=%p envState=%d envLevelCur=%d envFadeLevel=%f envFadeLevelNew=%f envFrameCount=%d envInterStep=%d",
    //            this, (int)envState, (int)envLevelCur, envFadeLevel, envFadeLevelNew, (int)envFrameCount, (int)envInterStep);

    panPrev = panCur;
    envFadeLevel = envFadeLevelNew;
}

void CGBChannel::applyVol()
{
    /* because agbplay generally wants to avoid the center panorama dilemma
     * (i.e. 0 pan not being in the center of the [-128,127] range)
     * we delay applying pan changes at the channel level, so we don't do the slightly
     * off center rounding for PCM channels. */

    int trkVolML = ((127 - pan) * vol) >> 8;
    int trkVolMR = ((pan + 128) * vol) >> 8;
    int chnVolL = (((127 - note.rhythmPan) * note.velocity) * trkVolML) >> 14;
    int chnVolR = (((note.rhythmPan + 128) * note.velocity) * trkVolMR) >> 14;
    assert(chnVolL >= 0);
    assert(chnVolR >= 0);

    /* strictly speaking the panorama also is affected by the sustain bug, but I'm too lazy to implement
     * is since most games probably won't be affected by this. */
    if (chnVolR / 2 >= chnVolL)
        this->panCur = Pan::RIGHT;
    else if (chnVolL / 2 >= chnVolR)
        this->panCur = Pan::LEFT;
    else
        this->panCur = Pan::CENTER;

    /* This envLevelCur assignment used to be below envSustain assignment and the only reason
     * it's now up here is because of a bug in the original mp2k where only 1 in 7 cases the
     * envelope sustain level would be applied correctly. */
    if (ctx.agbplaySoundMode.emulateCgbSustainBug) {
        if (!IsChn3() && mp2k_sus_vol_bug_update && envState == EnvState::SUS) {
            envLevelCur = envSustain;
            mp2k_sus_vol_bug_update = false;
        }
    }

    envPeak = static_cast<uint8_t>(std::clamp((chnVolL + chnVolR) >> 4, 0, 15));
    envSustain = static_cast<uint8_t>(std::clamp((envPeak * env.sus + 15) >> 4, 0, 15));
}

/*
 * public SquareChannel
 */

SquareChannel::SquareChannel(const MP2KContext &ctx, MP2KTrack *track, uint32_t instrDuty, ADSR env, Note note, uint8_t sweep)
    : CGBChannel(ctx, track, env, note)
      , sweep(sweep)
      , sweepEnabled(isSweepEnabled(sweep))
      , sweepConvergence(sweep2convergence(sweep))
      , sweepCoeff(sweep2coeff(sweep))
{
    static const float *patterns[4] = {
        CGBPatterns::pat_sq12,
        CGBPatterns::pat_sq25,
        CGBPatterns::pat_sq50,
        CGBPatterns::pat_sq75,
    };

    this->pat = patterns[instrDuty % 4];
    this->rs = std::make_unique<BlepResampler>();
}

void SquareChannel::SetPitch(int16_t pitch)
{
    // non original quality improving behavior
    if (!stop || freq <= 0.0f)
        freq = 3520.0f * powf(2.0f, float(note.midiKeyPitch - 69) * (1.0f / 12.0f) + float(pitch) * (1.0f / 768.0f));

    if (sweepEnabled && sweepStartCount < 0) {
        sweepTimer = freq2timer(freq / 8.0f);
        /* Because the initial frequency of a sweeped sound can only be set
         * in the beginning, we have to differentiate between the first and the other
         * SetPitch calls. */
        uint8_t time = sweepTime(sweep);
        assert(time != 0);
        sweepStartCount = static_cast<int16_t>(time * AGB_FPS * INTERFRAMES);
    }
}

void SquareChannel::Process(sample *buffer, size_t numSamples, MixingArgs& args)
{
    if (envState == EnvState::DEAD)
        return;
    stepEnvelope();
    if (envState == EnvState::DEAD)
        return;

    updateVolFade();

    if (numSamples == 0)
        return;

    VolumeFade vol = getVol();
    assert(pat);
    float lVolStep = (vol.toVolLeft - vol.fromVolLeft) * args.samplesPerBufferInv;
    float rVolStep = (vol.toVolRight - vol.fromVolRight) * args.samplesPerBufferInv;
    float lVol = vol.fromVolLeft;
    float rVol = vol.fromVolRight;
    float interStep;

    if (sweepEnabled) {
        //Debug::print("sweepTimer=%f sweepConvergence=%f sweepCoeff=%f resultFreq=%f",
        //        sweepTimer, sweepConvergence, sweepCoeff, timer2freq(sweepTimer));
        interStep = 8.0f * timer2freq(sweepTimer) * args.sampleRateInv;
    } else {
        interStep = freq * args.sampleRateInv;
    }

    float outBuffer[numSamples];

    rs->Process(outBuffer, numSamples, interStep, sampleFetchCallback, this);

    size_t i = 0;
    do {
        float samp = outBuffer[i++];
        buffer->left  += samp * lVol;
        buffer->right += samp * rVol;
        buffer++;
        lVol += lVolStep;
        rVol += rVolStep;
    } while (--numSamples > 0);

    if (sweepEnabled) {
        assert(sweepStartCount >= 0);
        if (sweepStartCount == 0) {
            sweepTimer *= sweepCoeff;
            if (isSweepAscending(sweep))
                sweepTimer = std::min(sweepTimer, sweepConvergence);
            else
                sweepTimer = std::max(sweepTimer, sweepConvergence);
        } else {
            sweepStartCount -= 128;
            if (sweepStartCount < 0)
                sweepStartCount = 0;
        }
    }
}

bool SquareChannel::sampleFetchCallback(std::vector<float>& fetchBuffer, size_t samplesRequired, void *cbdata)
{
    if (fetchBuffer.size() >= samplesRequired)
        return true;
    SquareChannel *_this = static_cast<SquareChannel *>(cbdata);
    size_t samplesToFetch = samplesRequired - fetchBuffer.size();
    size_t i = fetchBuffer.size();
    fetchBuffer.resize(samplesRequired);

    do {
        fetchBuffer[i++] = _this->pat[_this->pos++];
        _this->pos %= 8;
    } while (--samplesToFetch > 0);
    return true;
}

bool SquareChannel::isSweepEnabled(uint8_t sweep)
{
    if (sweep >= 0x80 || (sweep & 0x7) == 0)
        return false;
    else
        return true;
}

bool SquareChannel::isSweepAscending(uint8_t sweep)
{
    if (sweep & 8)
        return false;
    else
        return true;
}

float SquareChannel::sweep2coeff(uint8_t sweep)
{
    /* if sweep time is zero, don't change pitch */
    const int sweep_time = sweepTime(sweep);
    if (sweep_time == 0)
        return 1.0f;

    const int shifts = sweep & 7;
    float step_coeff;

    if (isSweepAscending(sweep)) {
        /* if ascending */
        step_coeff = static_cast<float>(128 + (128 >> shifts)) / 128.0f;
    } else {
        /* if descending */
        step_coeff = static_cast<float>(128 - (128 >> shifts)) / 128.0f;
    }

    /* convert the sweep pitch timer coefficient to the rate that agbplay runs at */
    const float hardware_sweep_rate = 128 / static_cast<float>(sweep_time);
    const float agbplay_sweep_rate = AGB_FPS * INTERFRAMES;
    const float coeff = powf(step_coeff, hardware_sweep_rate / agbplay_sweep_rate);

    return coeff;
}

float SquareChannel::sweep2convergence(uint8_t sweep)
{
    if (isSweepAscending(sweep)) {
        /* if ascending:
         *
         * Convergance is always at the maximum timer value to prevent hardware overflow */
        return 2047.0f;
    } else {
        /* if descending:
         *
         * Because hardware calculates sweep with:
         * timer -= timer >> shift
         * the timer converges to the value which timer >> shift always results zero. */
        const int shifts = sweep & 7;
        return static_cast<float>((1 << shifts) - 1);
    }
}

uint8_t SquareChannel::sweepTime(uint8_t sweep)
{
    return static_cast<uint8_t>((sweep & 0x70) >> 4);
}

/*
 * public WaveChannel
 */

WaveChannel::WaveChannel(const MP2KContext &ctx, MP2KTrack *track, uint32_t instrWave, ADSR env, Note note, bool useStairstep)
    : CGBChannel(ctx, track, env, note, useStairstep)
{
    static const uint8_t dummyWave[16] = {0};
    if (instrWave < AGB_MAP_ROM) {
        wavePtr = dummyWave;
    } else {
        instrWave -= AGB_MAP_ROM;
        if (ctx.rom.ValidRange(instrWave, 16))
            wavePtr = static_cast<const uint8_t *>(ctx.rom.GetPtr(instrWave));
        else
            wavePtr = dummyWave;
    }

    this->rs = std::make_unique<BlepResampler>();

    /* wave samples are unsigned by default, so we'll calculate the required
     * DC offset correction */
    float sum = 0.0f;
    for (int i = 0; i < 16; i++) {
        uint8_t twoNibbles = wavePtr[i];
        int nibbleA = twoNibbles >> 4;
        int nibbleB = twoNibbles & 0xF;
        sum += static_cast<float>(nibbleA) / 16.0f;
        sum += static_cast<float>(nibbleB) / 16.0f;
    }
    dcCorrection100 = -sum * (1.0f / 32.0f);

    if (ctx.agbplaySoundMode.accurateCh3Quantization) {
        sum = 0.0f;
        for (int i = 0; i < 16; i++) {
            uint8_t twoNibbles = wavePtr[i];
            int nibbleA = twoNibbles >> 4;
            int nibbleB = twoNibbles & 0xF;
            sum += static_cast<float>((nibbleA >> 2) + (nibbleA >> 1)) / 16.0f;
            sum += static_cast<float>((nibbleB >> 2) + (nibbleB >> 1)) / 16.0f;
        }
        dcCorrection75 = -sum * (1.0f / 32.0f);

        sum = 0.0f;
        for (int i = 0; i < 16; i++) {
            uint8_t twoNibbles = wavePtr[i];
            int nibbleA = twoNibbles >> 4;
            int nibbleB = twoNibbles & 0xF;
            sum += static_cast<float>((nibbleA >> 1)) / 16.0f;
            sum += static_cast<float>((nibbleB >> 1)) / 16.0f;
        }
        dcCorrection50 = -sum * (1.0f / 32.0f);

        sum = 0.0f;
        for (int i = 0; i < 16; i++) {
            uint8_t twoNibbles = wavePtr[i];
            int nibbleA = twoNibbles >> 4;
            int nibbleB = twoNibbles & 0xF;
            sum += static_cast<float>((nibbleA >> 2)) / 16.0f;
            sum += static_cast<float>((nibbleB >> 2)) / 16.0f;
        }
        dcCorrection25 = -sum * (1.0f / 32.0f);
    }
}

void WaveChannel::SetPitch(int16_t pitch)
{
    freq = (440.0f * 16.0f) * 
        powf(2.0f, float(note.midiKeyPitch - 69) * (1.0f / 12.0f) + float(pitch) * (1.0f / 768.0f));
}

void WaveChannel::Process(sample *buffer, size_t numSamples, MixingArgs& args)
{
    stepEnvelope();
    if (envState == EnvState::DEAD)
        return;

    updateVolFade();

    if (numSamples == 0)
        return;
    VolumeFade vol = getVol();

    float lVolStep = (vol.toVolLeft - vol.fromVolLeft) * args.samplesPerBufferInv;
    float rVolStep = (vol.toVolRight - vol.fromVolRight) * args.samplesPerBufferInv;
    float lVol = vol.fromVolLeft;
    float rVol = vol.fromVolRight;
    float interStep = freq * args.sampleRateInv;

    float outBuffer[numSamples];

    rs->Process(outBuffer, numSamples, interStep, sampleFetchCallback, this);

    size_t i = 0;
    do {
        float samp = outBuffer[i++];
        buffer->left  += samp * lVol;
        buffer->right += samp * rVol;
        buffer++;
        lVol += lVolStep;
        rVol += rVolStep;
    } while (--numSamples > 0);
}

bool WaveChannel::IsChn3() const
{
    return true;
}

VolumeFade WaveChannel::getVol() const
{
    auto retval = CGBChannel::getVol();

    if (!ctx.agbplaySoundMode.accurateCh3Volume) {
        return retval;
    }

    auto snapFunc = [](float x) {
        if (x < 1.5f / 32.0f)
            return 0.0f / 32.0f;
        else if (x < 5.5f / 32.0f)
            return 4.0f / 32.0f;
        else if (x < 9.5f / 32.0f)
            return 8.0f / 32.0f;
        else if (x < 13.5f / 32.0f)
            return 12.0f / 32.0f;
        else
            return 16.0f / 32.0f;
    };

    retval.fromVolLeft = snapFunc(retval.fromVolLeft);
    retval.fromVolRight = snapFunc(retval.fromVolRight);
    retval.toVolLeft = snapFunc(retval.toVolLeft);
    retval.toVolRight = snapFunc(retval.toVolRight);
    return retval;
}

bool WaveChannel::sampleFetchCallback(std::vector<float>& fetchBuffer, size_t samplesRequired, void *cbdata)
{
    if (fetchBuffer.size() >= samplesRequired)
        return true;
    WaveChannel *_this = static_cast<WaveChannel *>(cbdata);
    size_t samplesToFetch = samplesRequired - fetchBuffer.size();
    size_t i = fetchBuffer.size();
    fetchBuffer.resize(samplesRequired);

    if (_this->ctx.agbplaySoundMode.accurateCh3Quantization) {
        /* WARNING; VERY UGLY AND HACKY */
        VolumeFade fade = _this->getVol();
        const float fromVol = std::max(fade.fromVolLeft, fade.fromVolRight);
        const float toVol = std::max(fade.toVolLeft, fade.toVolRight);
        /* I'm not entirely certain whether the hardware uses arithmetic shifts or logic shifts (with bias).
         * Using logic shifts for now, hopefully works good enough... */
        auto mapFunc = [_this](float x, float& compensationScale, float& dcCorrection, uint32_t& shiftA, uint32_t& shiftB) {
            if (x < 6.0f / 32.0f) {
                shiftA = 2;
                shiftB = 4;
                compensationScale = 4.0f;
                dcCorrection = _this->dcCorrection25;
            } else if (x < 10.0f / 32.0f) {
                shiftA = 1;
                shiftB = 4;
                compensationScale = 2.0f;
                dcCorrection = _this->dcCorrection50;
            } else if (x < 14.0f / 32.0f) {
                shiftA = 1;
                shiftB = 2;
                compensationScale = 4.0f / 3.0f;
                dcCorrection = _this->dcCorrection75;
            } else {
                shiftA = 0;
                shiftB = 4;
                compensationScale = 1.0f;
                dcCorrection = _this->dcCorrection100;
            }
        };
        uint32_t shiftAFrom, shiftATo, shiftBFrom, shiftBTo;
        float compensationScaleFrom, compensationScaleTo;
        float dcCorrectionFrom, dcCorrectionTo;
        mapFunc(fromVol, compensationScaleFrom, dcCorrectionFrom, shiftAFrom, shiftBFrom);
        mapFunc(toVol, compensationScaleTo, dcCorrectionTo, shiftATo, shiftBTo);
        dcCorrectionFrom *= 16.0f;
        dcCorrectionTo *= 16.0f;

        float t = 0.0f;
        float t_inc = 1.0f / static_cast<float>(samplesToFetch);
        while (samplesToFetch-- > 0) {
            uint32_t pos = (_this->pos++) % 32;
            uint8_t nibble;
            if (pos % 2 == 0)
                nibble = static_cast<uint8_t>(_this->wavePtr[pos / 2] >> 4u);
            else
                nibble = static_cast<uint8_t>(_this->wavePtr[pos / 2] & 0xF);
            float sampleFrom = (static_cast<float>((nibble >> shiftAFrom) + (nibble >> shiftBFrom)) + dcCorrectionFrom) * compensationScaleFrom;
            float sampleTo   = (static_cast<float>((nibble >> shiftATo  ) + (nibble >> shiftBTo  )) + dcCorrectionTo  ) * compensationScaleTo;
            float sample = (sampleFrom + t * (sampleTo - sampleFrom)) * (1.0f / 16.0f);
            t += t_inc;
            fetchBuffer[i++] = sample;
        }
    } else {
        while (samplesToFetch-- > 0) {
            uint32_t pos = (_this->pos++) % 32;
            uint8_t nibble;
            if (pos % 2 == 0)
                nibble = static_cast<uint8_t>(_this->wavePtr[pos / 2] >> 4u);
            else
                nibble = static_cast<uint8_t>(_this->wavePtr[pos / 2] & 0xF);
            float sample = nibble * (1.0f / 16.0f) + _this->dcCorrection100;
            fetchBuffer[i++] = sample;
        }
    }

    return true;
}

/*
 * public NoiseChannel
 */

NoiseChannel::NoiseChannel(const MP2KContext &ctx, MP2KTrack *track, uint32_t instrNp, ADSR env, Note note)
    : CGBChannel(ctx, track, env, note)
{
    this->rs = std::make_unique<NearestResampler>();
    if ((instrNp & 0x1) == 0) {
        noiseState = 0x4000;
        noiseLfsrMask = 0x6000;
    } else {
        noiseState = 0x40;
        noiseLfsrMask = 0x60;
    }
}

void NoiseChannel::SetPitch(int16_t pitch)
{
    float fkey = note.midiKeyPitch + static_cast<float>(pitch) * (1.0f / 64.0f);
    float noisefreq;
    if (fkey < 76.0f)
        noisefreq = 4096.0f * powf(8.0f, (fkey - 60.0f) * (1.0f / 12.0f));
    else if (fkey < 78.0f)
        noisefreq = 65536.0f * powf(2.0f, (fkey - 76.0f) * (1.0f / 2.0f));
    else if (fkey < 80.0f)
        noisefreq = 131072.0f * powf(2.0f, (fkey - 78.0f));
    else
        noisefreq = 524288.0f;

    freq = std::max(4.5714f, noisefreq);
}

void NoiseChannel::Process(sample *buffer, size_t numSamples, MixingArgs& args)
{
    stepEnvelope();
    if (envState == EnvState::DEAD)
        return;

    updateVolFade();

    if (numSamples == 0)
        return;

    static const std::array<float, 4> noiseFreqs{
        32768.0f, 65536.0f, 131072.0f, 262144.0f,
    };
    const float noiseFreq = noiseFreqs[ctx.mp2kSoundMode.dacConfig % noiseFreqs.size()];

    VolumeFade vol = getVol();
    float lVolStep = (vol.toVolLeft - vol.fromVolLeft) * args.samplesPerBufferInv;
    float rVolStep = (vol.toVolRight - vol.fromVolRight) * args.samplesPerBufferInv;
    float lVol = vol.fromVolLeft;
    float rVol = vol.fromVolRight;
    float interStep = freq / noiseFreq;

    float outBuffer[numSamples];

    Resampler::ResamplerChainData rcd;
    rcd._this = rs.get();
    rcd.phaseInc = interStep;
    rcd.cbPtr = sampleFetchCallback;
    rcd.cbdata = this;

    srs.Process(outBuffer, numSamples,
            noiseFreq / float(STREAM_SAMPLERATE),
            Resampler::ResamplerChainSampleFetchCB, &rcd);

    size_t i = 0;
    do {
        float samp = outBuffer[i++];
        buffer->left  += samp * lVol;
        buffer->right += samp * rVol;
        buffer++;
        lVol += lVolStep;
        rVol += rVolStep;
    } while (--numSamples > 0);
}

bool NoiseChannel::sampleFetchCallback(std::vector<float>& fetchBuffer, size_t samplesRequired, void *cbdata)
{
    if (fetchBuffer.size() >= samplesRequired)
        return true;
    NoiseChannel *_this = static_cast<NoiseChannel *>(cbdata);
    size_t samplesToFetch = samplesRequired - fetchBuffer.size();
    size_t i = fetchBuffer.size();
    fetchBuffer.resize(samplesRequired);

    do {
        float sample;
        if (_this->noiseState & 1) {
            sample = 0.5f;
            _this->noiseState >>= 1;
            _this->noiseState ^= _this->noiseLfsrMask;
        } else {
            sample = -0.5f;
            _this->noiseState >>= 1;
        }
        fetchBuffer[i++] = sample;
    } while (--samplesToFetch > 0);

    return true;
}
