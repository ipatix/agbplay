#include <cmath>
#include <cassert>

#include "CGBChannel.h"
#include "CGBPatterns.h"
#include "MyException.h"
#include "Debug.h"
#include "Util.h"

#define WEIRD_CGB_ENV_FIX 2

using namespace std;
using namespace agbplay;

/*
 * public CGBChannel
 */

CGBChannel::CGBChannel()
{
    this->interPos = 0.0f;
    this->pos = 0;
    this->owner = nullptr;
    this->envInterStep = 0;
    this->leftVol = 0;
    this->rightVol = 0;
    this->envLevel = 0;
    this->fromLeftVol = 0;
    this->fromRightVol = 0;
    this->fromEnvLevel = 0;
    this->eState = EnvState::SUS;
}

CGBChannel::~CGBChannel()
{
}

void CGBChannel::Init(void *owner, CGBDef def, Note note, ADSR env)
{
    this->owner = owner;
    this->note = note;
    this->def = def;
    this->env.att = env.att & 0x7;
    this->env.dec = env.dec & 0x7;
    this->env.sus = env.sus & 0xF;
    this->env.rel = env.rel & 0x7;
    this->eState = EnvState::INIT;
}

void *CGBChannel::GetOwner()
{
    return owner;
}

float CGBChannel::GetFreq()
{
    return freq;
}

void CGBChannel::SetVol(uint8_t leftVol, uint8_t rightVol)
{
    // FIXME check for correct type conversion
    if (eState < EnvState::REL) {
#ifdef CGB_PAN_SNAP
        if (leftVol << 1 > rightVol) {
            // snap left
            this->leftVol = (leftVol + rightVol) >> 5;
            this->rightVol = 0;
        } else if (rightVol << 1 > leftVol) {
            // snap right
            this->rightVol = (leftVol + rightVol) >> 5;
            this->leftVol = 0;
        } else {
            // snap mid
            this->leftVol = this->rightVol = (leftVol + rightVol) >> 5;
        }
#else
        this->leftVol = leftVol >> 4;
        this->rightVol = rightVol >> 4;
#endif
    }
}

ChnVol CGBChannel::GetVol()
{
    float envBase = float(fromEnvLevel);
    uint32_t stepDiv;
    switch (eState) {
        case EnvState::ATK:
            stepDiv = env.att;
            break;
        case EnvState::DEC:
            stepDiv = env.dec;
            break;
        case EnvState::SUS:
            stepDiv = (env.dec == 0) ? 1 : env.dec;
            break;
        case EnvState::REL:
        case EnvState::DIE:
            stepDiv = env.rel;
            break;
        default:
            stepDiv = 1;
            break;
    }
    assert(stepDiv);
    float envDelta = (float(envLevel) - envBase) / float(INTERFRAMES / WEIRD_CGB_ENV_FIX * stepDiv) ;
    float finalFromEnv = envBase + envDelta * float(envInterStep);
    float finalToEnv = envBase + envDelta * float(envInterStep);
    return ChnVol(
            float(fromLeftVol) * finalFromEnv / 512.0f,
            float(fromRightVol) * finalFromEnv / 512.0f,
            float(leftVol) * finalToEnv / 512.0f,
            float(rightVol) * finalToEnv / 512.0f);
}

CGBDef CGBChannel::GetDef()
{
    return def;
}

uint8_t CGBChannel::GetMidiKey()
{
    return note.midiKey;
}

int8_t CGBChannel::GetNoteLength()
{
    return note.length;
}

void CGBChannel::Release()
{
    if (eState < EnvState::REL) {
        if (env.rel == 0) {
            envLevel = 0;
            eState = EnvState::DEAD;
        } else if (envLevel == 0) {
            // TODO check if this doesn't short cut releasing tones
            eState = EnvState::DEAD;
        } else {
            eState = EnvState::REL;
        }
    }
}

bool CGBChannel::TickNote()
{
    if (eState < EnvState::REL) {
        if (note.length > 0) {
            note.length--;
            if (note.length == 0) {
                if (envLevel == 0) {
                    eState = EnvState::DEAD;
                } else {
                    eState = EnvState::REL;
                }
                return false;
            }
            return true;
        } else if (note.length == -1) {
            return true;
        } else throw MyException("ShoundChannel::NoteTick shouldn't be able to crash");
    } else {
        return false;
    }
}

EnvState CGBChannel::GetState()
{
    return eState;
}

void CGBChannel::StepEnvelope()
{
    if (eState == EnvState::INIT) {
        fromLeftVol = leftVol;
        fromRightVol = rightVol;
        envInterStep = 0;
        if ((env.att | env.dec) == 0) {
            eState = EnvState::SUS;
            fromEnvLevel = env.sus;
            envLevel = env.sus;
            return;
        } else if (env.att == 0 && env.sus < 0xF) {
            eState = EnvState::DEC;
            fromEnvLevel = 0xF;
            envLevel = 0xE;
            return;
        } else if (env.att == 0) {
            eState = EnvState::DEC;
            fromEnvLevel = 0xF;
            envLevel = 0xF;
            return;
        } else {
            eState = EnvState::ATK;
            fromEnvLevel = 0x0;
            envLevel = 0x1;
            return;
        }
    }
    else if (eState == EnvState::ATK) {
        if (++envInterStep >= INTERFRAMES / WEIRD_CGB_ENV_FIX * env.att) {
            fromEnvLevel = envLevel;
            envInterStep = 0;
            if (++envLevel >= 0xF) {
                if (env.dec == 0) {
                    fromEnvLevel = env.sus;
                    envLevel = env.sus;
                    eState = EnvState::SUS;
                } else {
                    envLevel = 0xF;
                    eState = EnvState::DEC;
                }
            }
        }
    }
    else if (eState == EnvState::DEC) {
        if (++envInterStep >= INTERFRAMES / WEIRD_CGB_ENV_FIX * env.dec) {
            fromEnvLevel = envLevel;
            envInterStep = 0;
            if (--envLevel <= env.sus) {
                eState = EnvState::SUS;
            }
        }
    } 
    else if (eState == EnvState::SUS) {
        if (++envInterStep >= INTERFRAMES / WEIRD_CGB_ENV_FIX * env.dec) {
            fromEnvLevel = envLevel;
            envInterStep = 0;
        }
    }
    else if (eState == EnvState::REL) {
        if (++envInterStep >= INTERFRAMES / WEIRD_CGB_ENV_FIX * env.rel) {
            if (env.rel == 0) {
                fromEnvLevel = 0;
                envLevel = 0;
                eState = EnvState::DEAD;
            } else {
                fromEnvLevel = envLevel;
                envInterStep = 0;
                if (--envLevel <= 0) {
                    eState = EnvState::DIE;
                }
            }
        }
    } 
    else if (eState == EnvState::DIE) {
        if (++envInterStep >= INTERFRAMES / WEIRD_CGB_ENV_FIX * env.rel) {
            fromEnvLevel = envLevel;
            eState = EnvState::DEAD;
        }
    }
}


void CGBChannel::UpdateVolFade()
{
    fromLeftVol = leftVol;
    fromRightVol = rightVol;
}

const float *CGBChannel::GetPat()
{
    return pat;
}

/*
 * public SquareChannel
 */

SquareChannel::SquareChannel() : CGBChannel()
{
    this->pat = CGBPatterns::pat_sq50;
}

SquareChannel::~SquareChannel()
{
}

void SquareChannel::Init(void *owner, CGBDef def, Note note, ADSR env)
{
    CGBChannel::Init(owner, def, note, env);
    switch (def.wd) {
        case WaveDuty::D12:
            pat = CGBPatterns::pat_sq12;
            break;
        case WaveDuty::D25:
            pat = CGBPatterns::pat_sq25;
            break;
        case WaveDuty::D50:
            pat = CGBPatterns::pat_sq50;
            break;
        case WaveDuty::D75:
            pat = CGBPatterns::pat_sq75;
            break;
        default:
            throw MyException("Illegal Square Initializer");
    }
}

void SquareChannel::SetPitch(int16_t pitch)
{
    freq = 3520.0f * powf(2.0f, float(note.midiKey - 69) / 12.0f + float(pitch) / 768.0f);
}

/*
 * public WaveChannel
 */

WaveChannel::WaveChannel() : CGBChannel()
{
    for (int i = 0; i < 32; i++)
    {
        waveBuffer[i] = 0.0f;
    }
    this->pat = &waveBuffer[0];
}

WaveChannel::~WaveChannel()
{
}

void WaveChannel::Init(void *owner, CGBDef def, Note note, ADSR env)
{
    env.sus = (env.sus * 2) > 0xF ? 0xF : env.sus * 2;
    CGBChannel::Init(owner, def, note, env);

    float sum = 0.0f;
    for (size_t i = 0; i < 16; i++)
    {
        uint8_t twoNibbles = def.wavePtr[i];
        float first = float(twoNibbles >> 4) / 16.0f;
        sum += first;
        float second = float(twoNibbles & 0xF) / 16.0f;
        sum += second;
        waveBuffer[i*2] = first;
        waveBuffer[i*2+1] = second;
    }
    float dcCorrection = sum * 0.03125f;
    for (size_t i = 0; i < 32; i++)
    {
        waveBuffer[i] -= dcCorrection;
    }
}

void WaveChannel::SetPitch(int16_t pitch)
{
    freq = 7040.0f * powf(2.0f, float(note.midiKey - 69) / 12.0f + float(pitch) / 768.0f);
}

/*
 * public NoiseChannel
 */

NoiseChannel::NoiseChannel() : CGBChannel()
{
    this->pat = CGBPatterns::pat_noise_fine;
}

NoiseChannel::~NoiseChannel()
{
}

void NoiseChannel::Init(void *owner, CGBDef def, Note note, ADSR env)
{
    CGBChannel::Init(owner, def, note, env);
    switch (def.np) {
        case NoisePatt::ROUGH:
            pat = CGBPatterns::pat_noise_rough;
            break;
        case NoisePatt::FINE:
            pat = CGBPatterns::pat_noise_fine;
            break;
        default:
            throw MyException("Illegal Noise Pattern");
    }
}

void NoiseChannel::SetPitch(int16_t pitch)
{
    freq = minmax(4.0f, 4096.0f * powf(8.0f, float(note.midiKey - 60) / 12.0f + float(pitch) / 768.0f), 524288.0f);
}
