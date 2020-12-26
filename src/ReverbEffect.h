#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

#include "Types.h"

#define AGB_FPS 60

class ReverbEffect
{
public:
    ReverbEffect(uint8_t intesity, size_t streamRate, uint8_t numAgbBuffers);
    virtual ~ReverbEffect();
    void ProcessData(sample *buffer, size_t numSamples);
protected:
    virtual size_t processInternal(sample *buffer, size_t numSamples);
    size_t getBlocksPerBuffer() const;
    float intensity;
    //size_t streamRate;
    std::vector<sample> reverbBuffer;
    size_t bufferPos;
    size_t bufferPos2;
};

class ReverbGS1 : public ReverbEffect
{
public:
    ReverbGS1(uint8_t intensity, size_t streamRate, uint8_t numAgbBuffers);
    ~ReverbGS1() override;
protected:
    size_t processInternal(sample *buffer, size_t numSamples) override;
    size_t getBlocksPerGsBuffer() const;
    std::vector<sample> gsBuffer;
};

class ReverbGS2 : public ReverbEffect
{
public:
    ReverbGS2(uint8_t intesity, size_t streamRate, uint8_t numAgbBuffers,
            float rPrimFac, float rSecFac);
    ~ReverbGS2() override;
protected:
    size_t processInternal(sample *buffer, size_t numSamples) override;
    std::vector<sample> gs2Buffer;
    size_t gs2Pos;
    float rPrimFac, rSecFac;
};

class ReverbTest : public ReverbEffect
{
public:
    ReverbTest(uint8_t intesity, size_t streamRate, uint8_t numAgbBuffers);
    ~ReverbTest() override;
protected:
    size_t processInternal(sample *buffer, size_t numSamples) override;
};
