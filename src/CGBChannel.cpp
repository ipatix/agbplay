#include <cmath>
#include <cassert>
#include <algorithm>

#include "CGBChannel.h"
#include "CGBPatterns.h"
#include "Xcept.h"
#include "Debug.h"
#include "Util.h"
#include "Constants.h"
#include "ConfigManager.h"

/*
 * public CGBChannel
 */

CGBChannel::CGBChannel(ADSR env, Note note)
    : env(env), note(note)
{
    this->env.att &= 0x7;
    this->env.dec &= 0x7;
    this->env.sus &= 0xF;
    this->env.rel &= 0x7;
}

uint8_t CGBChannel::GetTrackIdx() const
{
    return note.trackIdx;
}

void CGBChannel::SetVol(uint8_t vol, int8_t pan)
{
    if (stop)
        return;

    /* CGB volume and pan aren't applied immediately due to the nature of being heavily
     * intertwined with the envelope handling. So save them and actually update them during envelope handling */
    this->vol = vol;
    this->pan = pan;
}

VolumeFade CGBChannel::getVol() const
{
    float envBase = static_cast<float>(envLevelPrev);
    float finalFromEnv = envBase + envGradient * static_cast<float>(envGradientFrame * INTERFRAMES + envInterStep);
    float finalToEnv = finalFromEnv + envGradient;

    VolumeFade retval;
    retval.fromVolLeft = (panPrev == Pan::RIGHT) ? 0.0f : finalFromEnv * (1.0f / 32.0f);
    retval.fromVolRight = (panPrev == Pan::LEFT) ? 0.0f : finalFromEnv * (1.0f / 32.0f);
    retval.toVolLeft = (panCur == Pan::RIGHT) ? 0.0f : finalToEnv * (1.0f / 32.0f);
    retval.toVolRight = (panCur == Pan::LEFT) ? 0.0f : finalToEnv * (1.0f / 32.0f);
    return retval;
}

const Note& CGBChannel::GetNote() const
{
    return note;
}

void CGBChannel::Release(bool fastRelease)
{
    this->stop = true;
    this->fastRelease = fastRelease;
}

bool CGBChannel::TickNote()
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

EnvState CGBChannel::GetState() const
{
    return envState;
}

bool CGBChannel::IsReleasing() const
{
    return stop;
}

bool CGBChannel::IsFastReleasing() const
{
    return fastRelease;
}

void CGBChannel::stepEnvelope()
{
    if (envState == EnvState::INIT) {
        if (stop) {
            envState = EnvState::DEAD;
            return;
        }

        applyVol();
        updateVolFade();

        envLevelCur = 0;
        envInterStep = 0;
        envState = EnvState::ATK;

        if (env.att > 0) {
            envLevelPrev = 0;
        } else if (env.dec > 0) {
            envLevelPrev = envPeak;
            envLevelCur = envPeak;
            if (envPeak > 0)
                envState = EnvState::DEC;
            else
                envState = EnvState::SUS;
        } else {
            envLevelPrev = envSustain;
            envLevelCur = envSustain;
            envState = EnvState::SUS;
        }
    } else {
        if (++envInterStep < INTERFRAMES)
            return;
        envInterStep = 0;

        assert(envFrameCount > 0);
        envFrameCount--;
        envGradientFrame++;
    }

    if (envState == EnvState::PSEUDO_ECHO) {
        assert(note.pseudoEchoLen != 0);
        if (--note.pseudoEchoLen == 0) {
            envState = EnvState::DIE;
            envLevelCur = 0;
        }
        envFrameCount = 1;
        envGradient = 0.0f;
    } else if (stop && envState < EnvState::REL) {
        if (fastRelease) {
            /* fast release is mostly inteded as hack in agbplay for quickly supressing notes
             * but still giving them a little time to fade out */
            goto fast_release;
        } else {
            envState = EnvState::REL;
            envFrameCount = env.rel;
            if (envLevelCur == 0 || envFrameCount == 0)
                goto pseudo_echo_start;
            envGradientFrame = 0;
            envLevelPrev = envLevelCur;
            goto release;
        }
    } else if (envFrameCount == 0) {
        applyVol();

        envGradientFrame = 0;
        envLevelPrev = envLevelCur;

        if (envState == EnvState::REL && fastRelease) {
fast_release:
            /* This case should only occur if a note was release normally but
             * then changes to fast release later. */
            envState = EnvState::DIE;
            envFrameCount = 1;
            envGradientFrame = 0;
            envLevelPrev = envLevelCur;
            envLevelCur = 0;
        } else if (envState == EnvState::REL) {
release:
            assert(envLevelCur > 0);
            envLevelCur--;
            assert((int8_t)envLevelCur >= 0);

            if (envLevelCur == 0) {
pseudo_echo_start:
                envLevelCur = static_cast<uint8_t>(((envPeak * note.pseudoEchoVol) + 0xFF) >> 8);
                if (envLevelCur != 0 && note.pseudoEchoLen != 0) {
                    envState = EnvState::PSEUDO_ECHO;
                    envFrameCount = 1;
                    envLevelPrev = envLevelCur;
                } else {
                    if (env.rel == 0) {
                        envState = EnvState::DEAD;
                        return;
                    } else {
                        envState = EnvState::DIE;
                        envLevelCur = 0;
                        envFrameCount = env.rel;
                    }
                }
            } else {
                envFrameCount = env.rel;
                assert(env.rel != 0);
            }
        } else if (envState == EnvState::SUS) {
            envLevelCur = envSustain;
            envFrameCount = 1;
        } else if (envState == EnvState::DEC) {
            envLevelCur--;
            assert((int8_t)envLevelCur >= 0);

            if (envLevelCur <= envSustain) {
                if (env.sus == 0) {
                    goto pseudo_echo_start;
                } else {
                    envState = EnvState::SUS;
                    envLevelCur = envSustain;
                }
            }
            envFrameCount = env.dec;
            assert(env.dec != 0);
        } else if (envState == EnvState::ATK) {
            envLevelCur++;

            if (envLevelCur >= envPeak) {
                envState = EnvState::DEC;
                envFrameCount = env.dec;
                if (envPeak == 0 || envFrameCount == 0) {
                    envState = EnvState::SUS;
                } else {
                    envLevelCur = envPeak;
                }
            }
            envFrameCount = env.att;
            assert(env.att != 0);
        } else if (envState == EnvState::DIE) {
            envState = EnvState::DEAD;
            return;
        }

        assert(envFrameCount != 0);
        envGradient = static_cast<float>(envLevelCur - envLevelPrev) / static_cast<float>(envFrameCount * INTERFRAMES);
    }

    //Debug::print("envState=%d envLevelCur=%d envLevelPrev=%d envFrameCount=%d envGradientFrame=%d envGradient=%f",
    //        (int)envState, (int)envLevelCur, (int)envLevelPrev, (int)envFrameCount, (int)envGradientFrame, envGradient);
}


void CGBChannel::updateVolFade()
{
    panPrev = panCur;
}

void CGBChannel::applyVol()
{
    int combinedPan = std::clamp(pan + note.rhythmPan, -64, +63);

    if (combinedPan < -21)
        this->panCur = Pan::LEFT;
    else if (combinedPan > 20)
        this->panCur = Pan::RIGHT;
    else
        this->panCur = Pan::CENTER;

    envPeak = std::clamp<uint8_t>(uint8_t((note.velocity * vol) >> 10), 0, 15);
    envSustain = std::clamp<uint8_t>(uint8_t((envPeak * env.sus + 15) >> 4), 0, 15);
    // TODO is this if below right???
    if (envState == EnvState::SUS)
        envLevelCur = envSustain;
}

/*
 * public SquareChannel
 */

SquareChannel::SquareChannel(WaveDuty wd, ADSR env, Note note)
    : CGBChannel(env, note)
{
    static const float *patterns[4] = {
        CGBPatterns::pat_sq12,
        CGBPatterns::pat_sq25,
        CGBPatterns::pat_sq50,
        CGBPatterns::pat_sq75,
    };

    this->pat = patterns[static_cast<int>(wd)];
    this->rs = std::make_unique<BlepResampler>();
}

void SquareChannel::SetPitch(int16_t pitch)
{
    freq = 3520.0f * powf(2.0f, float(note.midiKeyPitch - 69) * (1.0f / 12.0f) + float(pitch) * (1.0f / 768.0f));
}

void SquareChannel::Process(sample *buffer, size_t numSamples, MixingArgs& args)
{
    stepEnvelope();
    if (envState == EnvState::DEAD)
        return;
    if (numSamples == 0)
        return;

    VolumeFade vol = getVol();
    assert(pat);
    float lVolStep = (vol.toVolLeft - vol.fromVolLeft) * args.samplesPerBufferInv;
    float rVolStep = (vol.toVolRight - vol.fromVolRight) * args.samplesPerBufferInv;
    float lVol = vol.fromVolLeft;
    float rVol = vol.fromVolRight;
    float interStep = freq * args.sampleRateInv;

    // TODO add sweep functionality

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

    updateVolFade();
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

/*
 * public WaveChannel
 */

/* This LUT is currently unused. It's supposed to be more accurate to the original
 * but on the other hand it just doesn't sound as good as a linear curve */
uint8_t WaveChannel::volLut[] = {
    0, 0, 4, 4, 4, 4, 8, 8, 8, 8, 12, 12, 12, 12, 16, 16
};

WaveChannel::WaveChannel(const uint8_t *wavePtr, ADSR env, Note note)
    : CGBChannel(env, note)
{
    this->rs = std::make_unique<BlepResampler>();

    /* wave samples are unsigned by default, so we'll load them with
     * DC offset correction */
    float sum = 0.0f;
    for (int i = 0; i < 16; i++) {
        uint8_t twoNibbles = wavePtr[i];
        float first = static_cast<float>(twoNibbles >> 4) / 16.0f;
        sum += first;
        float second = static_cast<float>(twoNibbles & 0xF) / 16.0f;
        sum += second;
        this->waveBuffer[i * 2 + 0] = first;
        this->waveBuffer[i * 2 + 1] = second;
    }

    float dcCorrection = sum * (1.0f / 32.0f);
    for (size_t i = 0; i < 32; i++)
        this->waveBuffer[i] -= dcCorrection;
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

    updateVolFade();
}

VolumeFade WaveChannel::getVol() const
{
    GameConfig& cfg = ConfigManager::Instance().GetCfg();

    auto retval = CGBChannel::getVol();

    if (!cfg.GetAccurateCh3Volume()) {
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

    do {
        fetchBuffer[i++] = _this->waveBuffer[_this->pos++];
        _this->pos %= 32;
    } while (--samplesToFetch > 0);
    return true;
}

/*
 * public NoiseChannel
 */

NoiseChannel::NoiseChannel(NoisePatt np, ADSR env, Note note)
    : CGBChannel(env, note)
{
    this->rs = std::make_unique<NearestResampler>();
    this->np = np;
}

void NoiseChannel::SetPitch(int16_t pitch)
{
    float noisefreq = 4096.0f * powf(8.0f, float(note.midiKeyPitch - 60) * (1.0f / 12.0f) + float(pitch) * (1.0f / 768.0f));
    freq = std::clamp(noisefreq, 8.0f, 524288.0f);
}

void NoiseChannel::Process(sample *buffer, size_t numSamples, MixingArgs& args)
{
    stepEnvelope();
    if (envState == EnvState::DEAD)
        return;

    VolumeFade vol = getVol();
    float lVolStep = (vol.toVolLeft - vol.fromVolLeft) * args.samplesPerBufferInv;
    float rVolStep = (vol.toVolRight - vol.fromVolRight) * args.samplesPerBufferInv;
    float lVol = vol.fromVolLeft;
    float rVol = vol.fromVolRight;
    float interStep = freq / NOISE_SAMPLING_FREQ;

    float outBuffer[numSamples];

    Resampler::ResamplerChainData rcd;
    rcd._this = rs.get();
    rcd.phaseInc = interStep;
    rcd.cbPtr = sampleFetchCallback;
    rcd.cbdata = this;

    srs.Process(outBuffer, numSamples,
            NOISE_SAMPLING_FREQ / float(STREAM_SAMPLERATE),
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

    updateVolFade();
}

bool NoiseChannel::sampleFetchCallback(std::vector<float>& fetchBuffer, size_t samplesRequired, void *cbdata)
{
    if (fetchBuffer.size() >= samplesRequired)
        return true;
    NoiseChannel *_this = static_cast<NoiseChannel *>(cbdata);
    size_t samplesToFetch = samplesRequired - fetchBuffer.size();
    size_t i = fetchBuffer.size();
    fetchBuffer.resize(samplesRequired);

    if (_this->np == NoisePatt::FINE) {
        do {
            fetchBuffer[i++] = CGBPatterns::pat_noise_fine[_this->pos++] - 0.5f;
            _this->pos %= NOISE_FINE_LEN;
        } while (--samplesToFetch > 0);
    } else if (_this->np == NoisePatt::ROUGH) {
        do {
            fetchBuffer[i++] = CGBPatterns::pat_noise_rough[_this->pos++] - 0.5f;
            _this->pos %= NOISE_ROUGH_LEN;
        } while (--samplesToFetch > 0);
    }
    return true;
}
