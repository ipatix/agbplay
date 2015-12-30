#include <cmath>
#include <cassert>

#include "SoundChannel.h"

using namespace std;
using namespace agbplay;

/*
 * public SoundChannel
 */

SoundChannel::SoundChannel(void *owner, SampleInfo sInfo, ADSR env, Note note, uint8_t leftVol, uint8_t rightVol, int16_t pitch, bool fixed)
{
    this->owner = owner;
    this->note = note;
    this->env = env;
    this->sInfo = sInfo;
    this->interPos = 0.0f;
    SetVol(leftVol, rightVol);
    this->fixed = fixed;
    SetPitch(pitch);
    // if instant attack is ative directly max out the envelope to not cut off initial sound
    this->eState = EnvState::INIT;
}

SoundChannel::~SoundChannel()
{
}

void *SoundChannel::GetOwner()
{
    return owner;
}

float SoundChannel::GetFreq()
{
    return freq;
}

void SoundChannel::SetVol(uint8_t leftVol, uint8_t rightVol)
{
    this->leftVol = note.velocity * leftVol / 128;
    this->rightVol = note.velocity * rightVol / 128;
}

uint8_t SoundChannel::GetVolL()
{
    return leftVol;
}

uint8_t SoundChannel::GetVolR()
{
    return rightVol;
}

uint8_t SoundChannel::GetMidiKey()
{
    return note.midiKey;
}

void SoundChannel::Release()
{
    if (eState < EnvState::REL)
        eState = EnvState::REL;
}

void SoundChannel::SetPitch(int16_t pitch)
{
    freq = sInfo.midCfreq * powf(2.0f, float(note.midiKey - 60) / 12.0f + float(pitch) / 768.0f);
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
        } else assert(false);
    } else {
        return false;
    }
}

EnvState SoundChannel::GetState()
{
    return eState;
}

void SoundChannel::StepEnvelope()
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

