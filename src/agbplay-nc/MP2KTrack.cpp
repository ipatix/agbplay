#include "MP2KTrack.h"

MP2KTrack::MP2KTrack(size_t pos)
    : pos(pos)
{
    Init();
}

void MP2KTrack::Init()
{
    patternLevel = 0;
    modt = MODT::PITCH;
    lastCmd = 0;
    pitch = 0;
    lastNoteKey = 0;
    lastNoteVel = 0;
    lastNoteLen = 0;
    reptCount = 0;
    prog = PROG_UNDEFINED; // TODO replace this with an instrument definition like in original MP2K
    vol = 0;
    mod = 0;
    bendr = 2;
    priority = 0;
    lfos = 22;
    lfodl = 0;
    lfodlCount = 0;
    lfoPhase = 0;
    lfoValue = 0;
    pseudoEchoVol = 0;
    pseudoEchoLen = 0;
    delay = 0;
    pan = 0;
    bend = 0;
    tune = 0;
    keyShift = 0;
    muted = false;
    isRunning = true;
    updateVolume = false;
    updatePitch = false;
}

int16_t MP2KTrack::GetPitch()
{
    int p = tune + bend * bendr + keyShift * 64;
    if (modt == MODT::PITCH)
        p += lfoValue * 4;
    return static_cast<int16_t>(p);
}

uint16_t MP2KTrack::GetVol()
{
    int32_t v = vol << 1;
    if (modt == MODT::VOL)
        v = (v * (lfoValue + 128)) >> 7;
    return static_cast<uint16_t>(v);
}

int16_t MP2KTrack::GetPan()
{
    int p = pan << 1;
    if (modt == MODT::PAN)
        p += lfoValue;
    return static_cast<int16_t>(p);
}

void MP2KTrack::ResetLfoValue()
{
    lfoValue = 0;
    lfoPhase = 0;

    if (modt == MODT::PITCH)
        updatePitch = true;
    else
        updateVolume = true;
}

