#include <cmath>

#include "CGBChannel.h"
#include "MyException.h"

using namespace std;
using namespace agbplay;

/*
 * public CGBChannel
 */

CGBChannel::CGBChannel(CGBType t)
{
    this->cType = t;
    this->interPos = 0.0f;
    this->owner = nullptr;
    this->leftVol = 0;
    this->rightVol = 0;
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
    this->env = env;
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
    return ChnVol(
            float(fromLeftVol) * float(fromEnvLevel) / 256,
            float(fromRightVol) * float(fromEnvLevel) / 256,
            float(leftVol) * float(envLevel) / 256,
            float(rightVol) * float(envLevel) / 256);
}

CGBDef CGBChannel::GetDef()
{
    return def;
}

uint8_t CGBChannel::GetMidiKey()
{
    return note.midiKey;
}

void CGBChannel::Release()
{
    if (eState < EnvState::REL) {
        eState = EnvState::REL;
        envInterStep = 0; // TODO check if this doesn't mess up interpolation
    }
}

bool CGBChannel::TickNote()
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
            fromLeftVol = leftVol;
            fromRightVol = rightVol;
            if (env.att == 0) {
                fromEnvLevel = 0xF;
            } else {
                fromEnvLevel = 0x0;
            }
            envLevel = env.att;
            envInterStep = 0;
            eState = EnvState::ATK;
            break;
        case EnvState::ATK:
            if (++envInterStep >= INTERFRAMES * env.att) {
                fromEnvLevel = envLevel;
                envInterStep = 0;
                int newLevel = envLevel + 1;
                if (newLevel >= 0xF) {
                    eState = EnvState::DEC;
                    envLevel = 0xF;
                } else {
                    envLevel = uint8_t(newLevel);
                }
            }
            break;
        case EnvState::DEC:
            if (++envInterStep >= INTERFRAMES * env.dec) {
                fromEnvLevel = envLevel;
                envInterStep = 0;
                int newLevel = envLevel - 1;
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
            if (++envInterStep >= INTERFRAMES * env.rel) {
                fromEnvLevel = envLevel;
                envInterStep = 0;
                int newLevel = envLevel -1;
                if (newLevel <= 0) {
                    eState = EnvState::DIE;
                    envLevel = 0;
                }
            }
            break;
        case EnvState::DIE:
            if (++envInterStep >= INTERFRAMES * env.rel) {
                fromEnvLevel = envLevel;
                eState = EnvState::DEAD;
            }
            break;
        case EnvState::DEAD:
            break;
    }
}

void CGBChannel::UpdateVolFade()
{
    fromLeftVol = leftVol;
    fromRightVol = rightVol;
}

void CGBChannel::SetPitch(int16_t pitch)
{
    switch (cType) {
        case CGBType::SQ1:
        case CGBType::SQ2:
            freq = powf(2.0f, float(note.midiKey - 60) / 12.0f + float(pitch) / 768.0f);
            break;
        case CGBType::WAVE:
            freq = 16 * powf(2.0f, float(note.midiKey - 60) / 12.0f + float(pitch) / 768.0f);
        case CGBType::NOISE:
            freq = 4096 * powf(8.0f, float(note.midiKey - 60) / 12.0f + float(pitch) / 768.0f);

    }
}


