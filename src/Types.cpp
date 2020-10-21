#include "Types.h"

using namespace agbplay;

ReverbType agbplay::str2rev(const std::string& str)
{
    if (str == "gs1")
        return ReverbType::GS1;
    else if (str == "gs2")
        return ReverbType::GS2;
    else if (str == "mgat")
        return ReverbType::MGAT;
    else if (str == "test")
        return ReverbType::TEST;
    else if (str == "none")
        return ReverbType::NONE;
    return ReverbType::NORMAL;
}

std::string agbplay::rev2str(ReverbType t)
{
    if (t == ReverbType::GS1)
        return "gs1";
    else if (t == ReverbType::GS2)
        return "gs2";
    else if (t == ReverbType::MGAT)
        return "mgat";
    else if (t == ReverbType::TEST)
        return "test";
    else if (t == ReverbType::NONE)
        return "none";
    return "normal";
}

ResamplerType agbplay::str2res(const std::string& str)
{
    if (str == "nearest")
        return ResamplerType::NEAREST;
    else if (str == "linear")
        return ResamplerType::LINEAR;
    else if (str == "sinc")
        return ResamplerType::SINC;
    else if (str == "blep")
        return ResamplerType::BLEP;
    return ResamplerType::LINEAR;
}

std::string agbplay::res2str(ResamplerType t)
{
    if (t == ResamplerType::NEAREST)
        return "nearest";
    else if (t == ResamplerType::LINEAR)
        return "linear";
    else if (t == ResamplerType::SINC)
        return "sinc";
    else if (t == ResamplerType::BLEP)
        return "blep";
    return "linear";
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
