#include <cmath>
#include <cassert>
#include <string>
#include <algorithm>

#include "Constants.h"
#include "Debug.h"
#include "SoundChannel.h"
#include "Util.h"
#include "Xcept.h"
#include "ConfigManager.h"

/*
 * public SoundChannel
 */

SoundChannel::SoundChannel(SampleInfo sInfo, ADSR env, const Note& note, bool fixed)
    : env(env), note(note), sInfo(sInfo), fixed(fixed) 
{
    GameConfig& cfg = ConfigManager::Instance().GetCfg();
    ResamplerType t = fixed ? cfg.GetResTypeFixed() : cfg.GetResType();
    switch (t) {
    case ResamplerType::NEAREST:
        this->rs = std::make_unique<NearestResampler>();
        break;
    case ResamplerType::LINEAR:
        this->rs = std::make_unique<LinearResampler>();
        break;
    case ResamplerType::SINC:
        this->rs = std::make_unique<SincResampler>();
        break;
    case ResamplerType::BLEP:
        this->rs = std::make_unique<BlepResampler>();
        break;
    case ResamplerType::BLAMP:
        this->rs = std::make_unique<BlampResampler>();
        break;
    }

    // Golden Sun's synth instruments are marked by having a length of zero and a loop of zero
    if (sInfo.loopEnabled == true && sInfo.loopPos == 0 && sInfo.endPos == 0) {
        this->isGS = true;
    } else {
        this->isGS = false;
    }

    // Mario Power Tennis compressed instruments have a 'negative' length
    // strictly speaking, these are originally only available at 'fixed' frequency,
    // but we enhance song #17 which otherwise would have garbled/no sound
    if (sInfo.endPos >= 0x80000000) {
        this->isMPTcompressed = true;
        // flip it to it's intended length
        this->sInfo.endPos = -this->sInfo.endPos;
    } else {
        this->isMPTcompressed = false;
    }
    this->levelMPTcompressed = 0;
    this->shiftMPTcompressed = 0x38;
}

void SoundChannel::Process(sample *buffer, size_t numSamples, const MixingArgs& args)
{
    stepEnvelope();
    if (GetState() == EnvState::DEAD)
        return;
    if (numSamples == 0)
        return;

    float samplesPerBufferInv = 1.0f / float(numSamples);

    VolumeFade vol = getVol();
    vol.fromVolLeft *= args.vol;
    vol.fromVolRight *= args.vol;
    vol.toVolLeft *= args.vol;
    vol.toVolRight *= args.vol;

    ProcArgs cargs;
    cargs.lVolStep = (vol.toVolLeft - vol.fromVolLeft) * samplesPerBufferInv;
    cargs.rVolStep = (vol.toVolRight - vol.fromVolRight) * samplesPerBufferInv;
    cargs.lVol = vol.fromVolLeft;
    cargs.rVol = vol.fromVolRight;

    if (fixed && !isGS)
        cargs.interStep = float(args.fixedModeRate) * args.sampleRateInv;
    else 
        cargs.interStep = freq * args.sampleRateInv;

    if (isGS) {
        cargs.interStep /= 64.f; // different scale for GS
        // switch by GS type
        if (sInfo.samplePtr[1] == 0) {
            processModPulse(buffer, numSamples, cargs, samplesPerBufferInv);
        } else if (sInfo.samplePtr[1] == 1) {
            processSaw(buffer, numSamples, cargs);
        } else {
            processTri(buffer, numSamples, cargs);
        }
    } else {
        processNormal(buffer, numSamples, cargs);
    }
    updateVolFade();
}

uint8_t SoundChannel::GetTrackIdx() const
{
    return note.trackIdx;
}

void SoundChannel::SetVol(uint8_t vol, int8_t pan)
{
    if (!stop) {
        int combinedPan = std::clamp(pan + note.rhythmPan, -64, +63);
        this->leftVolCur = uint8_t(note.velocity * vol * (-combinedPan + 64) / 8192);
        this->rightVolCur = uint8_t(note.velocity * vol * (combinedPan + 64) / 8192);
    }
}

VolumeFade SoundChannel::getVol() const
{
    float envBase = float(envLevelPrev);
    float envDelta = (float(envLevelCur) - envBase) / float(INTERFRAMES);
    float finalFromEnv = envBase + envDelta * float(envInterStep);
    float finalToEnv = envBase + envDelta * float(envInterStep + 1);

    VolumeFade retval;
    retval.fromVolLeft = float(leftVolPrev) * finalFromEnv * (1.0f / 65536.0f);
    retval.fromVolRight = float(rightVolPrev) * finalFromEnv * (1.0f / 65536.0f);
    retval.toVolLeft = float(leftVolCur) * finalToEnv * (1.0f / 65536.0f);
    retval.toVolRight = float(rightVolCur) * finalToEnv * (1.0f / 65536.0f);
    return retval;
}

const Note& SoundChannel::GetNote() const
{
    return note;
}

void SoundChannel::Release()
{
    stop = true;
}

void SoundChannel::Kill()
{
    envState = EnvState::DEAD;
    envInterStep = 0;
}

void SoundChannel::SetPitch(int16_t pitch)
{
    // non original quality improving behavior
    if (!stop || freq <= 0.0f)
        freq = sInfo.midCfreq * powf(2.0f, float(note.midiKeyPitch - 60) * (1.0f / 12.0f) + float(pitch) * (1.0f / 768.0f));
}

bool SoundChannel::TickNote()
{
    if (!stop) {
        if (note.noteId != 0) {
            return true;
        }
        if (note.length > 0) {
            note.length--;
            if (note.length == 0) {
                stop = true;
                return false;
            }
        }
        return true;
    } else {
        return false;
    }
}

EnvState SoundChannel::GetState() const
{
    return envState;
}

void SoundChannel::stepEnvelope()
{
    if (envState == EnvState::INIT) {
        if (stop) {
            envState = EnvState::DEAD;
            return;
        }
        /* it's important to initialize the volume ramp here because in the constructor
         * the initial volume is not yet known (i.e. 0) */
        updateVolFade();

        /* Because we are smoothly fading all our amplitude changes, we avoid the case
         * where the fastest attack value will still cause a 16.6ms ramp instead of being
         * instant maximum amplitude. */
        if (env.att == 0xFF)
            envLevelPrev = 0xFF;
        else
            envLevelPrev = 0x0;

        envLevelCur = 0;
        envInterStep = 0;
        envState = EnvState::ATK;
    } else {
        /* On GBA, envelopes update every frame but because we do a multiple of updates per frame
         * (to increase timing accuracy of Note ONs), only every so many sub-frames we actually update
         * the envelope state. */
        if (++envInterStep < INTERFRAMES)
            return;
        envLevelPrev = envLevelCur;
        envInterStep = 0;
    }

    if (envState == EnvState::PSEUDO_ECHO) {
        assert(note.pseudoEchoLen != 0);
        if (--note.pseudoEchoLen == 0) {
            envState = EnvState::DIE;
            envLevelCur = 0;
        }
    } else if (stop) {
        if (envState == EnvState::DIE) {
            /* This is really just a transitional state that is supposed to be the last GBA frame
             * of fadeout. Because we smoothly ramp out envelopes, out envelopes are actually one
             * frame longer than on hardware- As soon as this state is reached the channel is disabled */
            envState = EnvState::DEAD;
        } else {
            envLevelCur = static_cast<uint8_t>((envLevelCur * env.rel) >> 8);
            /* ORIGINAL "BUG":
             * Even when pseudo echo has no length, the following condition will kick in and may cause
             * an earlier then intended note release */
            if (envLevelCur <= note.pseudoEchoVol) {
release:
                if (note.pseudoEchoVol == 0 || note.pseudoEchoLen == 0) {
                    envState = EnvState::DIE;
                    envLevelCur = 0;
                } else {
                    envState = EnvState::PSEUDO_ECHO;
                    envLevelCur = note.pseudoEchoVol;
                }
            }
        }
    } else {
        if (envState == EnvState::DEC) {
            envLevelCur = static_cast<uint8_t>((envLevelCur * env.dec) >> 8);
            if (envLevelCur <= env.sus) {
                envLevelCur = env.sus;
                if (envLevelCur == 0)
                    goto release;
                envState = EnvState::SUS;
            }
        } else if (envState == EnvState::ATK) {
            uint32_t newLevel = envLevelCur + env.att;
            if (newLevel >= 0xFF) {
                envLevelCur = 0xFF;
                envState = EnvState::DEC;
            } else {
                envLevelCur = static_cast<uint8_t>(newLevel);
            }
        }
    }
}

void SoundChannel::updateVolFade()
{
    leftVolPrev = leftVolCur;
    rightVolPrev = rightVolCur;
}

/*
 * private SoundChannel
 */

void SoundChannel::processNormal(sample *buffer, size_t numSamples, ProcArgs& cargs) {
    if (numSamples == 0)
        return;
    float outBuffer[numSamples];

    bool running;
    if (this->isMPTcompressed) {
        running = rs->Process(outBuffer, numSamples, cargs.interStep, sampleFetchCallbackMPTDecomp, this);
    } else {
        running = rs->Process(outBuffer, numSamples, cargs.interStep, sampleFetchCallback, this);
    }

    size_t i = 0;
    do {
        float samp = outBuffer[i++];

        buffer->left  += samp * cargs.lVol;
        buffer->right += samp * cargs.rVol;
        buffer++;
        cargs.lVol += cargs.lVolStep;
        cargs.rVol += cargs.rVolStep;
    } while (--numSamples > 0);
    if (!running)
        Kill();
}

void SoundChannel::processModPulse(sample *buffer, size_t numSamples, ProcArgs& cargs, float nBlocksReciprocal)
{
#define DUTY_BASE 2
#define DUTY_STEP 3
#define DEPTH 4
#define INIT_DUTY 5
    uint32_t fromPos;

    if (envInterStep == 0)
        fromPos = pos += uint32_t(sInfo.samplePtr[DUTY_STEP] << 24);
    else
        fromPos = pos;

    uint32_t toPos = fromPos + uint32_t(sInfo.samplePtr[DUTY_STEP] << 24);

    auto calcThresh = [](uint32_t val, uint8_t base, uint8_t depth, uint8_t init) {
        uint32_t iThreshold = uint32_t(init << 24) + val;
        iThreshold = int32_t(iThreshold) < 0 ? ~iThreshold >> 8 : iThreshold >> 8;
        iThreshold = iThreshold * depth + uint32_t(base << 24);
        return float(iThreshold) / float(0x100000000);
    };

    float fromThresh = calcThresh(fromPos, (uint8_t)sInfo.samplePtr[DUTY_BASE], (uint8_t)sInfo.samplePtr[DEPTH], (uint8_t)sInfo.samplePtr[INIT_DUTY]);
    float toThresh = calcThresh(toPos, (uint8_t)sInfo.samplePtr[DUTY_BASE], (uint8_t)sInfo.samplePtr[DEPTH], (uint8_t)sInfo.samplePtr[INIT_DUTY]);

    float deltaThresh = toThresh - fromThresh;
    float baseThresh = fromThresh + (deltaThresh * (float(envInterStep) * (1.0f / float(INTERFRAMES))));
    float threshStep = deltaThresh * (1.0f / float(INTERFRAMES)) * nBlocksReciprocal;
    float fThreshold = baseThresh;
#undef DUTY_BASE
#undef DUTY_STEP
#undef DEPTH
#undef INIT_DUTY

    do {
        float baseSamp = interPos < fThreshold ? 0.5f : -0.5f;
        // correct dc offset
        baseSamp += 0.5f - fThreshold;
        fThreshold += threshStep;
        buffer->left  += baseSamp * cargs.lVol;
        buffer->right += baseSamp * cargs.rVol;
        buffer++;

        cargs.lVol += cargs.lVolStep;
        cargs.rVol += cargs.rVolStep;

        interPos += cargs.interStep;
        // this below might glitch for too high frequencies, which usually shouldn't be used anyway
        if (interPos >= 1.0f) interPos -= 1.0f;
    } while (--numSamples > 0);
}

void SoundChannel::processSaw(sample *buffer, size_t numSamples, ProcArgs& cargs)
{
    const uint32_t fix = 0x70;

    do {
        /*
         * Sorry that the baseSamp calculation looks ugly.
         * For accuracy it's a 1 to 1 translation of the original assembly code
         * Could probably be reimplemented easier. Not sure if it's a perfect saw wave
         */
        interPos += cargs.interStep;
        if (interPos >= 1.0f) interPos -= 1.0f;
        uint32_t var1 = uint32_t(interPos * 256) - fix;
        uint32_t var2 = uint32_t(interPos * 65536.0f) << 17;
        uint32_t var3 = var1 - (var2 >> 27);
        pos = var3 + uint32_t(int32_t(pos) >> 1);

        float baseSamp = float((int32_t)pos) / 256.0f;

        buffer->left  += baseSamp * cargs.lVol;
        buffer->right += baseSamp * cargs.rVol;
        buffer++;

        cargs.lVol += cargs.lVolStep;
        cargs.rVol += cargs.rVolStep;
    } while (--numSamples > 0);
}

void SoundChannel::processTri(sample *buffer, size_t numSamples, ProcArgs& cargs)
{
    do {
        interPos += cargs.interStep;
        if (interPos >= 1.0f) interPos -= 1.0f;
        float baseSamp;
        if (interPos < 0.5f) {
            baseSamp = (4.0f * interPos) - 1.0f;
        } else {
            baseSamp = 3.0f - (4.0f * interPos);
        }

        buffer->left  += baseSamp * cargs.lVol;
        buffer->right += baseSamp * cargs.rVol;
        buffer++;

        cargs.lVol += cargs.lVolStep;
        cargs.rVol += cargs.rVolStep;
    } while (--numSamples > 0);
}

bool SoundChannel::sampleFetchCallback(std::vector<float>& fetchBuffer, size_t samplesRequired, void *cbdata)
{
    if (fetchBuffer.size() >= samplesRequired)
        return true;
    SoundChannel *_this = static_cast<SoundChannel *>(cbdata);
    size_t samplesToFetch = samplesRequired - fetchBuffer.size();
    size_t i = fetchBuffer.size();
    fetchBuffer.resize(samplesRequired);

    do {
        size_t samplesTilLoop = _this->sInfo.endPos - _this->pos;
        size_t thisFetch = std::min(samplesTilLoop, samplesToFetch);

        samplesToFetch -= thisFetch;
        do {
            fetchBuffer[i++] = float(_this->sInfo.samplePtr[_this->pos++]) / 128.0f;
        } while (--thisFetch > 0);

        if (_this->pos >= _this->sInfo.endPos) {
            if (_this->sInfo.loopEnabled) {
                _this->pos = _this->sInfo.loopPos;
            } else {
                std::fill(fetchBuffer.begin() + i, fetchBuffer.end(), 0.0f);
                return false;
            }
        }
    } while (samplesToFetch > 0);
    return true;
}

bool SoundChannel::sampleFetchCallbackMPTDecomp(std::vector<float>& fetchBuffer, size_t samplesRequired, void *cbdata)
{
    if (fetchBuffer.size() >= samplesRequired)
        return true;
    SoundChannel *_this = static_cast<SoundChannel *>(cbdata);
    size_t samplesToFetch = samplesRequired - fetchBuffer.size();
    size_t i = fetchBuffer.size();
    fetchBuffer.resize(samplesRequired);

    do {
        size_t samplesTilLoop = _this->sInfo.endPos - _this->pos;
        size_t thisFetch = std::min(samplesTilLoop, samplesToFetch);

        samplesToFetch -= thisFetch;
        do {
            // once again, I just took over the assembly implementation
            // there is probably plenty of room to make this nicer, but it at least works for now
            bool loNibble = _this->pos & 1;
            uint32_t samplePos = _this->pos++ >> 1u;
            int8_t data = _this->sInfo.samplePtr[samplePos];

            // 4 bit nibble is shifted up to bit 31..28
            int32_t nibble;
            if (loNibble)
                nibble = (int32_t(data) << 28) & 0xF0000000;
            else
                nibble = (int32_t(data) << 24) & 0xF0000000;

            // in the ARM ASM you can easily just shift by more than 31, but this does not work on x86/C++
            if (_this->shiftMPTcompressed <= 63) {
                int32_t actualShift = (int32_t)(_this->shiftMPTcompressed >> 1u);
                _this->levelMPTcompressed = int16_t(_this->levelMPTcompressed + (nibble >> actualShift));
            }

            if (nibble & 0x80000000)
                nibble = -nibble;
            _this->shiftMPTcompressed = uint8_t(_this->shiftMPTcompressed + 4);
            _this->shiftMPTcompressed = uint8_t((uint32_t)_this->shiftMPTcompressed - ((uint32_t)nibble >> 28u));

            fetchBuffer[i++] = float(_this->levelMPTcompressed) / 128.0f;
        } while (--thisFetch > 0);

        if (_this->pos >= _this->sInfo.endPos) {
            // MPT compressed sample cannot loop
            std::fill(fetchBuffer.begin() + i, fetchBuffer.end(), 0.0f);
            return false;
        }
    } while (samplesToFetch > 0);
    return true;
}
