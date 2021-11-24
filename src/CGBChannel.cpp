#include <cmath>
#include <cassert>
#include <algorithm>

#include "CGBChannel.h"
#include "CGBPatterns.h"
#include "Xcept.h"
#include "Debug.h"
#include "Util.h"
#include "Constants.h"

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
    int combinedPan = std::clamp(pan + note.rhythmPan, -64, +63);

    if (envState < EnvState::REL) {
        if (combinedPan < -21) {
            // snap left
            this->panCur = Pan::LEFT;
        } else if (combinedPan > 20) {
            // snap right
            this->panCur = Pan::RIGHT;
        } else {
            // snap mid
            this->panCur = Pan::CENTER;
        }
        envPeak = std::clamp<uint8_t>(uint8_t((note.velocity * vol) >> 10), 0, 15);
        envSustain = std::clamp<uint8_t>(uint8_t((envPeak * env.sus + 15) >> 4), 0, 15);
        if (envState == EnvState::SUS)
            envLevelCur = envSustain;
    }
}

VolumeFade CGBChannel::getVol() const
{
    float envBase = float(envLevelPrev);
    uint32_t stepDiv;
    switch (envState) {
        case EnvState::ATK:
            stepDiv = env.att;
            break;
        case EnvState::DEC:
            stepDiv = env.dec;
            break;
        case EnvState::SUS:
            stepDiv = 1;
            break;
        case EnvState::CGB_FAST_REL:
        case EnvState::REL:
        case EnvState::DIE:
            stepDiv = env.rel;
            break;
        default:
            throw Xcept("Getting volume of invalid state: %d", (int)envState);
    }
    assert(stepDiv);
    float envDelta = (float(envLevelCur) - envBase) / float(INTERFRAMES * stepDiv);
    float finalFromEnv = envBase + envDelta * float(envInterStep);
    float finalToEnv = envBase + envDelta * float(envInterStep + 1);

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
    if (fastRelease && envState < EnvState::CGB_FAST_REL) {
        if (env.rel == 0) {
            envLevelCur = 0;
            envState = EnvState::DEAD;
        } else if (envLevelCur == 0 && envLevelPrev == 0) {
            envState = EnvState::DEAD;
        } else {
            envStateNext = EnvState::CGB_FAST_REL;
        }
    } else if (envState < EnvState::REL) {
        if (env.rel == 0) {
            envLevelCur = 0;
            envState = EnvState::DEAD;
        } else if (envLevelCur == 0 && envLevelPrev == 0) {
            envState = EnvState::DEAD;
        } else {
            envStateNext = EnvState::REL;
        }
    }
}

bool CGBChannel::TickNote()
{
    if (envState < EnvState::REL) {
        if (note.length > 0) {
            note.length--;
            if (note.length == 0) {
                if (envLevelCur == 0) {
                    envState = EnvState::DEAD;
                } else {
                    envState = EnvState::REL;
                }
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

EnvState CGBChannel::GetNextState() const
{
    return envStateNext;
}

void CGBChannel::stepEnvelope()
{
    switch (envState) {
        case EnvState::INIT:
            envStateNext = EnvState::ATK;
            panPrev = panCur;
            envInterStep = 0;
            if ((env.att | env.dec) == 0 || (envSustain == 0 && envPeak == 0)) {
                envState = EnvState::SUS;
                envLevelPrev = envSustain;
                envLevelCur = envSustain;
                return;
            } else if (env.att == 0 && env.sus < 0xF) {
                envState = EnvState::DEC;
                envLevelPrev = envPeak;
                envLevelCur = uint8_t(std::clamp(envPeak - 1, 0, 15));
                if (envLevelCur < envSustain) envLevelCur = envSustain;
                return;
            } else if (env.att == 0) {
                envState = EnvState::SUS;
                envLevelPrev = envSustain;
                envLevelCur = envSustain;
                return;
            } else {
                envState = EnvState::ATK;
                envLevelPrev = 0x0;
                envLevelCur = 0x1;
                return;
            }
            break;
        case EnvState::ATK:
            assert(env.att);
            if (++envInterStep >= INTERFRAMES * env.att) {
                if (envStateNext == EnvState::DEC) {
                    envState = EnvState::DEC;
                    goto Ldec;
                }
                if (envStateNext == EnvState::SUS) {
                    envState = EnvState::SUS;
                    goto Lsus;
                }
                if (envStateNext == EnvState::REL) {
                    envState = EnvState::REL;
                    goto Lrel;
                }
                if (envStateNext == EnvState::CGB_FAST_REL) {
                    envState = EnvState::CGB_FAST_REL;
                    goto Lfast_rel;
                }
                envLevelPrev = envLevelCur;
                envInterStep = 0;
                if (++envLevelCur >= envPeak) {
                    if (env.dec == 0) {
                        //envLevelCur = envSustain;
                        envStateNext = EnvState::SUS;
                    } else if (envPeak == envSustain) {
                        envStateNext = EnvState::SUS;
                        envLevelCur = envPeak;
                    } else {
                        envLevelCur = envPeak;
                        envStateNext = EnvState::DEC;
                    }
                }
            }
            break;
        case EnvState::DEC:
            assert(env.dec);
            if (++envInterStep >= INTERFRAMES * env.dec) {
                if (envStateNext == EnvState::SUS) {
                    envState = EnvState::SUS;
                    goto Lsus;
                }
                if (envStateNext == EnvState::REL) {
                    envState = EnvState::REL;
                    goto Lrel;
                }
                if (envStateNext == EnvState::CGB_FAST_REL) {
                    envState = EnvState::CGB_FAST_REL;
                    goto Lfast_rel;
                }
Ldec:
                envLevelPrev = envLevelCur;
                envInterStep = 0;
                if (int(envLevelCur - 1) <= int(envSustain)) {
                    envLevelCur = envSustain;
                    envStateNext = EnvState::SUS;
                } else {
                    envLevelCur = uint8_t(std::clamp(envLevelCur - 1, 0, 15));
                }
            }
            break;
        case EnvState::SUS:
            if (++envInterStep >= INTERFRAMES) {
                if (envStateNext == EnvState::REL) {
                    envState = EnvState::REL;
                    goto Lrel;
                }
                if (envStateNext == EnvState::CGB_FAST_REL) {
                    envState = EnvState::CGB_FAST_REL;
                    goto Lfast_rel;
                }
Lsus:
                envLevelPrev = envLevelCur;
                envInterStep = 0;
            }
            break;
        case EnvState::REL:
            if (++envInterStep >= INTERFRAMES * env.rel) {
                if (envStateNext == EnvState::DIE) {
                    goto Ldie;
                }
                if (envStateNext == EnvState::CGB_FAST_REL) {
                    envState = EnvState::CGB_FAST_REL;
                    goto Lfast_rel;
                }
Lrel:
                if (env.rel == 0) {
                    envLevelPrev = 0;
                    envLevelCur = 0;
                    envState = EnvState::DEAD;
                } else {
                    envLevelPrev = envLevelCur;
                    envInterStep = 0;
                    if (envLevelCur - 1 <= 0) {
                        envStateNext = EnvState::DIE;
                        envLevelCur = 0;
                    } else {
                        envLevelCur--;
                    }
                }
            }
            break;
        case EnvState::CGB_FAST_REL:
            if (++envInterStep >= INTERFRAMES * env.rel) {
                if (envStateNext == EnvState::DIE) {
                    goto Ldie;
                }
Lfast_rel:
                if (env.rel == 0) {
                    envLevelPrev = 0;
                    envLevelCur = 0;
                    envState = EnvState::DEAD;
                } else {
                    envLevelPrev = envLevelCur;
                    envInterStep = 0;
                    envLevelCur = 0;
                    envStateNext = EnvState::DIE;
                }
            }
            break;
        case EnvState::DIE:
Ldie:
            envState = EnvState::DEAD;
            break;
        default:
            break;
    }
}


void CGBChannel::updateVolFade()
{
    panPrev = panCur;
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
