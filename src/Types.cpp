#include "Types.h"

using namespace agbplay;

/*
 * ChnVol
 */

ChnVol::ChnVol(float fromVolLeft, float fromVolRight, 
        float toVolLeft, float toVolRight)
{
    this->fromVolLeft = fromVolLeft;
    this->fromVolRight = fromVolRight;
    this->toVolLeft = toVolLeft;
    this->toVolRight = toVolRight;
}

ChnVol::ChnVol()
{
}

/*
 * ADSR
 */

ADSR::ADSR(uint8_t att, uint8_t dec, uint8_t sus, uint8_t rel)
{
    this->att = att;
    this->dec = dec;
    this->sus = sus;
    this->rel = rel;
}

ADSR::ADSR()
{
    this->att = 0xFF;
    this->dec = 0x00;
    this->sus = 0xFF;
    this->rel = 0x00;
}

/*
 * Note
 */

Note::Note(uint8_t midiKey, uint8_t velocity, int8_t length)
{
    this->originalKey = this->midiKey = midiKey;
    this->velocity = velocity;
    this->length = length;
}

Note::Note()
{
}

/*
 * public SampleInfo
 */

SampleInfo::SampleInfo(int8_t *samplePtr, float midCfreq, bool loopEnabled, uint32_t loopPos, uint32_t endPos)
{
    this->samplePtr = samplePtr;
    this->midCfreq = midCfreq;
    this->loopPos = loopPos;
    this->endPos = endPos;
    this->loopEnabled = loopEnabled;
}

SampleInfo::SampleInfo()
{
}
