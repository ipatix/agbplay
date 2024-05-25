#include "MP2KTrack.h"

MP2KTrack::MP2KTrack(size_t pos)
    : pos(pos)
{
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

