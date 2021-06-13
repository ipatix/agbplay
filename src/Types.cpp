#include "Types.h"

ReverbType str2rev(const std::string& str)
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

std::string rev2str(ReverbType t)
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

ResamplerType str2res(const std::string& str)
{
    if (str == "nearest")
        return ResamplerType::NEAREST;
    else if (str == "linear")
        return ResamplerType::LINEAR;
    else if (str == "sinc")
        return ResamplerType::SINC;
    else if (str == "blep")
        return ResamplerType::BLEP;
    else if (str == "blamp")
        return ResamplerType::BLAMP;
    return ResamplerType::LINEAR;
}

std::string res2str(ResamplerType t)
{
    if (t == ResamplerType::NEAREST)
        return "nearest";
    else if (t == ResamplerType::LINEAR)
        return "linear";
    else if (t == ResamplerType::SINC)
        return "sinc";
    else if (t == ResamplerType::BLEP)
        return "blep";
    else if (t == ResamplerType::BLAMP)
        return "blamp";
    return "linear";
}

CGBPolyphony str2cgbPoly(const std::string& str)
{
    if (str == "mono-strict")
        return CGBPolyphony::MONO_STRICT;
    else if (str == "mono-smooth")
        return CGBPolyphony::MONO_SMOOTH;
    else if (str == "poly")
        return CGBPolyphony::POLY;
    return CGBPolyphony::MONO_STRICT;
}

std::string cgbPoly2str(CGBPolyphony t) {
    if (t == CGBPolyphony::MONO_STRICT)
        return "mono-strict";
    else if (t == CGBPolyphony::MONO_SMOOTH)
        return "mono-smooth";
    else if (t == CGBPolyphony::POLY)
        return "poly";
    return "mono-strict";
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

SampleInfo::SampleInfo(const int8_t *samplePtr, float midCfreq, bool loopEnabled, uint32_t loopPos, uint32_t endPos)
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

/*
 * public EnginePars
 */

EnginePars::EnginePars(uint8_t vol, uint8_t rev, uint8_t freq)
{
    this->vol = vol;
    this->rev = rev;
    this->freq = freq;
}
