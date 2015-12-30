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
#ifdef CGB_PAN_SNAP
    if (leftVol << 1 > rightVol) {
        // snap left
        this->leftVol = (leftVol + rightVol) >> 5;
        this->rightVol = 0;
    } else if (rightVol << 1 > leftVol) {
        // snap right
        this->rightVol= (leftVol + rightVol) >> 5;
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

uint8_t CGBChannel::GetVolL()
{
    return leftVol;
}

uint8_t CGBChannel::GetVolR()
{
    return rightVol;
}

uint8_t CGBChannel::GetMidiKey()
{
    return note.midiKey;
}

void CGBChannel::Release()
{
    if (eState < EnvState::REL)
        eState = EnvState::REL;
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
            processLeftVol = leftVol;
            processRightVol = rightVol;
            if (env.att == 0xFF) {
                processEnvLevel = 0xFF;
            } else {
                processEnvLevel = 0x0;
            }
            envLevel = env.att;
            envInterStep = 0;
            eState = EnvState::ATK;
            break;
        case EnvState::ATK:
            if (++envInterStep >= INTERFRAMES) {
                processEnvLevel = envLevel;
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
                processEnvLevel = envLevel;
                envInterStep = 0;
                int newLevel = envLevel * env.dec / 256;
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
                processEnvLevel = envLevel;
                envInterStep = 0;
            }
            break;
        case EnvState::REL:
            if (++envInterStep >= INTERFRAMES) {
                processEnvLevel = envLevel;
                envInterStep = 0;
                int newLevel = envLevel * env.rel;
                if (newLevel <= 0) {
                    eState = EnvState::DIE;
                    envLevel = 0;
                }
            }
            break;
        case EnvState::DIE:
            if (++envInterStep >= INTERFRAMES) {
                processEnvLevel = envLevel;
                eState = EnvState::DEAD;
            }
            break;
        case EnvState::DEAD:
            break;
    }
}

void CGBChannel::SetPitch(int16_t pitch)
{
    freq = powf(2.0f, float(note.midiKey - 60) / 12.0f + float(pitch) / 768.0f);
}


