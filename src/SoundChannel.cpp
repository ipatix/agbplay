#include <cmath>
#include <cassert>
#include <string>
#include <algorithm>

#include "Debug.h"
#include "SoundChannel.h"
#include "Util.h"
#include "Xcept.h"

using namespace agbplay;

/*
 * public SoundChannel
 */

SoundChannel::SoundChannel(uint8_t owner, SampleInfo sInfo, ADSR env, Note note, uint8_t vol, int8_t pan, int16_t pitch, bool fixed)
{
    this->owner = owner;
    this->note = note;
    this->env = env;
    this->sInfo = sInfo;
    this->eState = EnvState::INIT;
    this->envInterStep = 0;
    SetVol(vol, pan);
    this->fixed = fixed;
    // TODO g++ (Debian 8.2.0-13) will say for make_unique that memory is not included???
    if (fixed)
        this->rs = std::unique_ptr<Resampler>(new NearestResampler());
    else
        this->rs = std::unique_ptr<Resampler>(new LinearResampler());
    this->interPos = 0.0f;
    SetPitch(pitch);
    // if instant attack is ative directly max out the envelope to not cut off initial sound
    this->pos = 0;
    if (sInfo.loopEnabled == true && sInfo.loopPos == 0 && sInfo.endPos == 0) {
        this->isGS = true;
    } else {
        this->isGS = false;
    }
}

SoundChannel::~SoundChannel()
{
}

void SoundChannel::Process(float *buffer, size_t nblocks, const MixingArgs& args)
{
    stepEnvelope();
    if (GetState() == EnvState::DEAD)
        return;
    if (nblocks == 0)
        return;

    float nBlocksReciprocal = 1.f / float(nblocks);

    ChnVol vol = getVol();
    vol.fromVolLeft *= args.vol;
    vol.fromVolRight *= args.vol;
    vol.toVolLeft *= args.vol;
    vol.toVolRight *= args.vol;

    ProcArgs cargs;
    cargs.lVolStep = (vol.toVolLeft - vol.fromVolLeft) * nBlocksReciprocal;
    cargs.rVolStep = (vol.toVolRight - vol.fromVolRight) * nBlocksReciprocal;
    cargs.lVol = vol.fromVolLeft;
    cargs.rVol = vol.fromVolRight;

    if (fixed && !isGS)
        cargs.interStep = float(args.fixedModeRate) * args.sampleRateReciprocal;
    else 
        cargs.interStep = freq * args.sampleRateReciprocal;

    if (isGS) {
        cargs.interStep /= 64.f; // different scale for GS
        // switch by GS type
        if (sInfo.samplePtr[1] == 0) {
            processModPulse(buffer, nblocks, cargs, nBlocksReciprocal);
        } else if (sInfo.samplePtr[1] == 1) {
            processSaw(buffer, nblocks, cargs);
        } else {
            processTri(buffer, nblocks, cargs);
        }
    } else {
        processNormal(buffer, nblocks, cargs);
    }
    updateVolFade();
}

uint8_t SoundChannel::GetOwner()
{
    return owner;
}

void SoundChannel::SetVol(uint8_t vol, int8_t pan)
{
    if (eState < EnvState::REL) {
        this->leftVol = uint8_t(note.velocity * vol * (-pan + 64) / 8192);
        this->rightVol = uint8_t(note.velocity * vol * (pan + 64) / 8192);
    }
}

ChnVol SoundChannel::getVol()
{
    float envBase = float(fromEnvLevel);
    float envDelta = (float(envLevel) - envBase) / float(INTERFRAMES);
    float finalFromEnv = envBase + envDelta * float(envInterStep);
    float finalToEnv = envBase + envDelta * float(envInterStep + 1);
    return ChnVol(
            float(fromLeftVol) * finalFromEnv * (1.0f / 65536.0f),
            float(fromRightVol) * finalFromEnv * (1.0f / 65536.0f),
            float(leftVol) * finalToEnv * (1.0f / 65536.0f),
            float(rightVol) * finalToEnv * (1.0f / 65536.0f));
}

uint8_t SoundChannel::GetMidiKey()
{
    return note.originalKey;
}

int8_t SoundChannel::GetNoteLength()
{
    return note.length;
}

void SoundChannel::Release()
{
    if (eState < EnvState::REL) {
        eState = EnvState::REL;
    }
}

void SoundChannel::Kill()
{
    eState = EnvState::DEAD;
    envInterStep = 0;
}

void SoundChannel::SetPitch(int16_t pitch)
{
    freq = sInfo.midCfreq * powf(2.0f, float(note.midiKey - 60) * (1.0f / 12.0f) + float(pitch) * (1.0f / 768.0f));
}

bool SoundChannel::TickNote()
{
    if (eState < EnvState::REL) {
        if (note.length > 0) {
            note.length--;
            if (note.length == 0) {
                eState = EnvState::REL;
                return false;
            }
            return true;
        } else if (note.length == -1) {
            return true;
        } else throw Xcept("Illegal Note countdown: %d", (int)note.length);
    } else {
        return false;
    }
}

EnvState SoundChannel::GetState()
{
    return eState;
}

SampleInfo& SoundChannel::GetInfo()
{
    return sInfo;
}

uint8_t SoundChannel::GetInterStep()
{
    return envInterStep;
}

void SoundChannel::stepEnvelope()
{
    switch (eState) {
    case EnvState::INIT:
        fromLeftVol = leftVol;
        fromRightVol = rightVol;
        if (env.att == 0xFF) {
            fromEnvLevel = 0xFF;
        } else {
            fromEnvLevel = 0x0;
        }
        envLevel = env.att;
        envInterStep = 0;
        eState = EnvState::ATK;
        break;
    case EnvState::ATK:
        if (++envInterStep >= INTERFRAMES) {
            fromEnvLevel = envLevel;
            envInterStep = 0;
            int newLevel = envLevel + env.att;
            if (newLevel >= 0xFF) {
                eState = EnvState::DEC;
                envLevel = 0xFF;
            } else {
                envLevel = uint8_t(newLevel);
            }
        }
        break;
    case EnvState::DEC:
        if (++envInterStep >= INTERFRAMES) {
            fromEnvLevel = envLevel;
            envInterStep = 0;
            int newLevel = (envLevel * env.dec) >> 8;
            if (newLevel <= env.sus) {
                eState = EnvState::SUS;
                envLevel = env.sus;
            } else {
                envLevel = uint8_t(newLevel);
            }
        }
        break;
    case EnvState::SUS:
        if (++envInterStep >= INTERFRAMES) {
            fromEnvLevel = envLevel;
            envInterStep = 0;
        }
        break;
    case EnvState::REL:
        if (++envInterStep >= INTERFRAMES) {
            fromEnvLevel = envLevel;
            envInterStep = 0;
            int newLevel = (envLevel * env.rel) >> 8;
            if (newLevel <= 0) {
                eState = EnvState::DIE;
                envLevel = 0;
            } else {
                envLevel = uint8_t(newLevel);
            }
        }
        break;
    case EnvState::DIE:
        if (++envInterStep >= INTERFRAMES) {
            fromEnvLevel = envLevel;
            eState = EnvState::DEAD;
        }
        break;
    case EnvState::DEAD:
        break;
    }
}

void SoundChannel::updateVolFade()
{
    fromLeftVol = leftVol;
    fromRightVol = rightVol;
}

/*
 * private SoundChannel
 */

void SoundChannel::processNormal(float *buffer, size_t nblocks, ProcArgs& cargs) {
    if (nblocks == 0)
        return;
    float outBuffer[nblocks];

    bool running = rs->Process(outBuffer, nblocks, cargs.interStep, sampleFetchCallback, this);

    size_t i = 0;
    do {
        float samp = outBuffer[i++];

        *buffer++ += samp * cargs.lVol;
        *buffer++ += samp * cargs.rVol;
        cargs.lVol += cargs.lVolStep;
        cargs.rVol += cargs.rVolStep;
    } while (--nblocks > 0);
    if (!running)
        Kill();
}

void SoundChannel::processModPulse(float *buffer, size_t nblocks, ProcArgs& cargs, float nBlocksReciprocal)
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
        *buffer++ += baseSamp * cargs.lVol;
        *buffer++ += baseSamp * cargs.rVol;

        cargs.lVol += cargs.lVolStep;
        cargs.rVol += cargs.rVolStep;

        interPos += cargs.interStep;
        // this below might glitch for too high frequencies, which usually shouldn't be used anyway
        if (interPos >= 1.0f) interPos -= 1.0f;
    } while (--nblocks > 0);
}

void SoundChannel::processSaw(float *buffer, size_t nblocks, ProcArgs& cargs)
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

        *buffer++ += baseSamp * cargs.lVol;
        *buffer++ += baseSamp * cargs.rVol;

        cargs.lVol += cargs.lVolStep;
        cargs.rVol += cargs.rVolStep;
    } while (--nblocks > 0);
}

void SoundChannel::processTri(float *buffer, size_t nblocks, ProcArgs& cargs)
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

        *buffer++ += baseSamp * cargs.lVol;
        *buffer++ += baseSamp * cargs.rVol;

        cargs.lVol += cargs.lVolStep;
        cargs.rVol += cargs.rVolStep;
    } while (--nblocks > 0);
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
