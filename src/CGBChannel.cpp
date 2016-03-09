#include <cmath>
#include <cassert>

#include "CGBChannel.h"
#include "CGBPatterns.h"
#include "MyException.h"
#include "Debug.h"
#include "Util.h"

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
    this->envLevel = 0;
    this->fromEnvLevel = 0;
    this->envPeak = 0;
    this->envSustain = 0;
    this->pan = Pan::CENTER;
    this->fromPan = Pan::CENTER;
    this->eState = EnvState::DEAD;
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

void CGBChannel::SetVol(uint8_t vol, int8_t pan)
{
    if (eState < EnvState::REL) {
        if (pan < -32) {
            // snap left
            this->pan = Pan::LEFT;
        } else if (pan > 32) {
            // snap right
            this->pan = Pan::RIGHT;
        } else {
            // snap mid
            this->pan = Pan::CENTER;
        }
        envPeak = minmax<uint8_t>(0, uint8_t((note.velocity * vol) >> 10), 15);
        envSustain = minmax<uint8_t>(0, uint8_t((envPeak * env.sus + 15) >> 4), 15);
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
            stepDiv = 1;
            break;
        case EnvState::REL:
        case EnvState::DIE:
            stepDiv = env.rel;
            break;
        default:
            throw MyException(FormatString("Getting volume of invalid state: %d", (int)eState));
    }
    assert(stepDiv);
    float envDelta = (float(envLevel) - envBase) / float(INTERFRAMES * stepDiv);
    float finalFromEnv = envBase + envDelta * float(envInterStep);
    float finalToEnv = envBase + envDelta * float(envInterStep + 1);
    return ChnVol(
            (fromPan == Pan::RIGHT) ? 0.0f : finalFromEnv * (1.0f / 32.0f),
            (fromPan == Pan::LEFT) ? 0.0f : finalFromEnv * (1.0f / 32.0f),
            (fromPan == Pan::RIGHT) ? 0.0f : finalToEnv * (1.0f / 32.0f),
            (fromPan == Pan::LEFT) ? 0.0f : finalToEnv * (1.0f / 32.0f));
}

CGBDef CGBChannel::GetDef()
{
    return def;
}

uint8_t CGBChannel::GetMidiKey()
{
    return note.originalKey;
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
        } else if (envLevel == 0 && fromEnvLevel == 0) {
            eState = EnvState::DEAD;
        } else {
            nextState = EnvState::REL;
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
    switch (eState) {
        case EnvState::INIT:
            nextState = EnvState::ATK;
            fromPan = pan;
            envInterStep = 0;
            if ((env.att | env.dec) == 0 || (envSustain == 0 && envPeak == 0)) {
                eState = EnvState::SUS;
                fromEnvLevel = envSustain;
                envLevel = envSustain;
                return;
            } else if (env.att == 0 && env.sus < 0xF) {
                eState = EnvState::DEC;
                fromEnvLevel = envPeak;
                envLevel = uint8_t(minmax(0, envPeak - 1, 15));
                if (envLevel < envSustain) envLevel = envSustain;
                return;
            } else if (env.att == 0) {
                eState = EnvState::SUS;
                fromEnvLevel = envSustain;
                envLevel = envSustain;
                return;
            } else {
                eState = EnvState::ATK;
                fromEnvLevel = 0x0;
                envLevel = 0x1;
                return;
            }
            break;
        case EnvState::ATK:
            assert(env.att);
            if (++envInterStep >= INTERFRAMES * env.att) {
                if (nextState == EnvState::DEC) {
                    eState = EnvState::DEC;
                    goto Ldec;
                }
                if (nextState == EnvState::SUS) {
                    eState = EnvState::SUS;
                    goto Lsus;
                }
                if (nextState == EnvState::REL) {
                    eState = EnvState::REL;
                    goto Lrel;
                }
                fromEnvLevel = envLevel;
                envInterStep = 0;
                if (++envLevel >= envPeak) {
                    if (env.dec == 0) {
                        //envLevel = envSustain;
                        nextState = EnvState::SUS;
                    } else if (envPeak == envSustain) {
                        nextState = EnvState::SUS;
                        envLevel = envPeak;
                    } else {
                        envLevel = envPeak;
                        nextState = EnvState::DEC;
                    }
                }
            }
            break;
        case EnvState::DEC:
            assert(env.dec);
            if (++envInterStep >= INTERFRAMES * env.dec) {
                if (nextState == EnvState::SUS) {
                    eState = EnvState::SUS;
                    goto Lsus;
                }
                if (nextState == EnvState::REL) {
                    eState = EnvState::REL;
                    goto Lrel;
                }
Ldec:
                fromEnvLevel = envLevel;
                envInterStep = 0;
                if (int(envLevel - 1) <= int(envSustain)) {
                    envLevel = envSustain;
                    nextState = EnvState::SUS;
                } else {
                    envLevel = uint8_t(minmax(0, envLevel - 1, 15));
                }
            }
            break;
        case EnvState::SUS:
            if (++envInterStep >= INTERFRAMES) {
                if (nextState == EnvState::REL) {
                    eState = EnvState::REL;
                    goto Lrel;
                }
                if (nextState == EnvState::REL) {
                    eState = EnvState::REL;
                    goto Lrel;
                }
Lsus:
                fromEnvLevel = envLevel;
                envInterStep = 0;
            }
            break;
        case EnvState::REL:
            if (++envInterStep >= INTERFRAMES * env.rel) {
                if (nextState == EnvState::DIE) {
                    goto Ldie;
                }
Lrel:
                if (env.rel == 0) {
                    fromEnvLevel = 0;
                    envLevel = 0;
                    eState = EnvState::DEAD;
                } else {
                    fromEnvLevel = envLevel;
                    envInterStep = 0;
                    if (envLevel - 1 <= 0) {
                        nextState = EnvState::DIE;
                        envLevel = 0;
                    } else {
                        envLevel--;
                    }
                }
            }
            break;
        case EnvState::DIE:
Ldie:
            eState = EnvState::DEAD;
            break;
        default:
            break;
    }
}


void CGBChannel::UpdateVolFade()
{
    fromPan = pan;
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

uint8_t WaveChannel::volLut[] = {
    0, 0, 4, 4, 4, 4, 8, 8, 8, 8, 12, 12, 12, 12, 16, 16
};

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
    //env.sus = (env.sus * 2) > 0xF ? 0xF : uint8_t(env.sus * 2);
    CGBChannel::Init(owner, def, note, env);

    float sum = 0.0f;
    for (size_t i = 0; i < 16; i++)
    {
        uint8_t twoNibbles = def.wavePtr[i];
        float first = float(twoNibbles >> 4) * (1.0f / 16.0f);
        sum += first;
        float second = float(twoNibbles & 0xF) * (1.0f / 16.0f);
        sum += second;
        waveBuffer[i*2] = first;
        waveBuffer[i*2+1] = second;
    }
    float dcCorrection = sum * (1.0f / 32.0f);
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
    pos = 0;
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
    freq = minmax(8.0f, 4096.0f * powf(8.0f, float(note.midiKey - 60) / 12.0f + float(pitch) / 768.0f), 524288.0f);
}
