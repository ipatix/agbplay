#include "Types.h"

using namespace agbplay;

ReverbType agbplay::str2rev(const std::string& str)
{
    if (str == "GS1")
        return ReverbType::GS1;
    else if (str == "GS2")
        return ReverbType::GS2;
    else if (str == "MGAT")
        return ReverbType::MGAT;
    else if (str == "TEST")
        return ReverbType::TEST;
    else if (str == "NONE")
        return ReverbType::NONE;
    return ReverbType::NORMAL;
}

std::string agbplay::rev2str(ReverbType t)
{
    if (t == ReverbType::GS1)
        return "GS1";
    else if (t == ReverbType::GS2)
        return "GS2";
    else if (t == ReverbType::MGAT)
        return "MGAT";
    else if (t == ReverbType::TEST)
        return "TEST";
    else if (t == ReverbType::NONE)
        return "NONE";
    return "NORMAL";
}

ResamplerType agbplay::str2res(const std::string& str)
{
    if (str == "NEAREST")
        return ResamplerType::NEAREST;
    else if (str == "LINEAR")
        return ResamplerType::LINEAR;
    else if (str == "SINC")
        return ResamplerType::SINC;
    else if (str == "BLEP")
        return ResamplerType::BLEP;
    return ResamplerType::LINEAR;
}

std::string agbplay::res2str(ResamplerType t)
{
    if (t == ResamplerType::NEAREST)
        return "NEAREST";
    else if (t == ResamplerType::LINEAR)
        return "LINEAR";
    else if (t == ResamplerType::SINC)
        return "SINC";
    else if (t == ResamplerType::BLEP)
        return "BLEP";
    return "LINEAR";
}

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
