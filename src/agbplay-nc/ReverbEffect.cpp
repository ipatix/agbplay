#include <algorithm>
#include <cassert>

#include "ReverbEffect.h"
#include "Util.h"
#include "Constants.h"
#include "Xcept.h"

/*
 * public ReverbEffect
 */

ReverbEffect::ReverbEffect(uint8_t intensity, size_t streamRate, uint8_t numAgbBuffers)
    : reverbBuffer((streamRate / AGB_FPS) * numAgbBuffers, sample{0.0f, 0.0f})
{
    SetLevel(intensity);
    const size_t bufferLen = streamRate / AGB_FPS;
    bufferPos = 0;
    bufferPos2 = bufferLen;
}

ReverbEffect::~ReverbEffect()
{
}

void ReverbEffect::ProcessData(sample *buffer, size_t numSamples)
{
    while (numSamples > 0)
    {
        size_t left = processInternal(buffer, numSamples);
        buffer += (numSamples - left);
        numSamples = left;
    }
}

void ReverbEffect::SetLevel(uint8_t level)
{
    intensity = level / 128.0f;
}

std::unique_ptr<ReverbEffect> ReverbEffect::MakeReverb(ReverbType reverbType, uint8_t intensity, size_t sampleRate, uint8_t numDmaBuffers)
{
    switch (reverbType) {
    case ReverbType::NORMAL:
        return std::make_unique<ReverbEffect>(
                intensity, sampleRate, numDmaBuffers);
    case ReverbType::NONE:
        return std::make_unique<ReverbEffect>(
                0, sampleRate, numDmaBuffers);
    case ReverbType::GS1:
        return std::make_unique<ReverbGS1>(
                intensity, sampleRate, numDmaBuffers);
    case ReverbType::GS2:
        return std::make_unique<ReverbGS2>(
                intensity, sampleRate, numDmaBuffers,
                0.4140625f, -0.0625f);
        // Mario Power Tennis uses same coefficients as Mario Golf Advance Tour
    case ReverbType::MGAT:
        return std::make_unique<ReverbGS2>(
                intensity, sampleRate, numDmaBuffers,
                0.25f, -0.046875f);
    case ReverbType::TEST:
        return std::make_unique<ReverbTest>(
                intensity, sampleRate, numDmaBuffers);
    default:
        throw Xcept("MakeReverb: Invalid Reverb Effect: {}", static_cast<int>(reverbType));
    }
}

/*
 * protected ReverbEffect
 */

size_t ReverbEffect::getBlocksPerBuffer() const
{
    return reverbBuffer.size();
}

size_t ReverbEffect::processInternal(sample *buffer, size_t numSamples)
{
    assert(numSamples > 0);
    std::vector<sample>& rbuf = reverbBuffer;
    size_t count = std::min(std::min(getBlocksPerBuffer() - bufferPos2, getBlocksPerBuffer() - bufferPos), numSamples);
    bool reset = false, reset2 = false;
    if (getBlocksPerBuffer() - bufferPos == count) {
        reset = true;
    }
    if (getBlocksPerBuffer() - bufferPos2 == count) {
        reset2 = true;
    } 
    size_t c = count;
    do {
        float rev = (rbuf[bufferPos].left + rbuf[bufferPos].right + 
                rbuf[bufferPos2].left + rbuf[bufferPos2].right) * intensity * (1.0f / 4.0f);
        rbuf[bufferPos].left  = buffer->left  += rev;
        rbuf[bufferPos].right = buffer->right += rev;
        buffer++;
        bufferPos++;
        bufferPos2++;
    } while (--c > 0);
    if (reset2) bufferPos2 = 0;
    if (reset) bufferPos = 0;
    return numSamples - count;
}

/*
 * ReverbGS1
 */

ReverbGS1::ReverbGS1(uint8_t intensity, size_t streamRate, uint8_t numAgbBuffers)
    : ReverbEffect(intensity, streamRate, numAgbBuffers), 
    gsBuffer((streamRate / AGB_FPS), sample{0.0f, 0.0f})
{
    bufferPos2 = 0;
}

ReverbGS1::~ReverbGS1()
{
}

size_t ReverbGS1::getBlocksPerGsBuffer() const
{
    return gsBuffer.size();
}

size_t ReverbGS1::processInternal(sample *buffer, size_t numSamples)
{
    std::vector<sample>& rbuf = reverbBuffer;
    const size_t bPerBuf = getBlocksPerBuffer();
    const size_t bPerGsBuf = getBlocksPerGsBuffer();
    size_t count = std::min(std::min(bPerBuf - bufferPos, bPerGsBuf - bufferPos2), numSamples);
    bool reset = false, resetGS = false;

    if (count == bPerBuf - bufferPos)
        reset = true;
    if (count == bPerGsBuf - bufferPos2)
        resetGS = true;

    size_t c = count;
    do {
        float mixL = buffer->left  + gsBuffer[bufferPos2].left;
        float mixR = buffer->right + gsBuffer[bufferPos2].right;

        float lA = rbuf[bufferPos].left;
        float rA = rbuf[bufferPos].right;

        buffer->left  = rbuf[bufferPos].left  = mixL;
        buffer->right = rbuf[bufferPos].right = mixR;

        float lRMix = 0.25f * mixL + 0.25f * rA;
        float rRMix = 0.25f * mixR + 0.25f * lA;

        gsBuffer[bufferPos2].left  = lRMix;
        gsBuffer[bufferPos2].right = rRMix;

        buffer++;

        bufferPos++;
        bufferPos2++;
    } while (--c > 0);

    if (resetGS) bufferPos2 = 0;
    if (reset) bufferPos = 0;
    return numSamples - count;
}

/*
 * ReverbGS2
 */

ReverbGS2::ReverbGS2(uint8_t intensity, size_t streamRate, uint8_t numAgbBuffers,
        float rPrimFac, float rSecFac)
    : ReverbEffect(intensity, streamRate, numAgbBuffers), 
    gs2Buffer(streamRate / AGB_FPS, sample{0.0f, 0.0f})
{
    // equivalent to the offset of -0xB0 samples for a 0x210 buffer size
    bufferPos2 = getBlocksPerBuffer() - (gs2Buffer.size() / 3);
    gs2Pos = 0;
    this->rPrimFac = rPrimFac;
    this->rSecFac = rSecFac;
}

ReverbGS2::~ReverbGS2()
{
}

size_t ReverbGS2::processInternal(sample *buffer, size_t numSamples)
{
    assert(numSamples > 0);
    std::vector<sample>& rbuf = reverbBuffer;
    size_t count = std::min(
            std::min(getBlocksPerBuffer() - bufferPos2, getBlocksPerBuffer() - bufferPos), 
            std::min(numSamples, gs2Buffer.size() - gs2Pos)
            );
    bool reset = false, reset2 = false, resetgs2 = false;

    if (getBlocksPerBuffer() - bufferPos2 == count) {
        reset2 = true;
    } 
    if (getBlocksPerBuffer() - bufferPos == count) {
        reset = true;
    }
    if ((gs2Buffer.size() / 2) - gs2Pos == count) {
        resetgs2 = true;
    }

    size_t c = count;
    do {
        float mixL = buffer->left  + gs2Buffer[gs2Pos].left;
        float mixR = buffer->right + gs2Buffer[gs2Pos].right;

        float lA = rbuf[bufferPos].left;
        float rA = rbuf[bufferPos].right;

        buffer->left  = rbuf[bufferPos].left  = mixL;
        buffer->right = rbuf[bufferPos].right = mixR;

        float lRMix = lA * rPrimFac + rA * rSecFac;
        float rRMix = rA * rPrimFac + lA * rSecFac;

        float lB = rbuf[bufferPos2].right * 0.25f;
        float rB = mixR * 0.25f;

        gs2Buffer[gs2Pos].left  = lRMix + lB;
        gs2Buffer[gs2Pos].right = rRMix + rB;

        buffer++;

        bufferPos++;
        bufferPos2++;
        gs2Pos++;
    } while (--c > 0);
    if (reset2) bufferPos2 = 0;
    if (reset) bufferPos = 0;
    if (resetgs2) gs2Pos = 0;
    return numSamples - count;
}

/*
 * ReverbTest
 */

ReverbTest::ReverbTest(uint8_t intensity, size_t streamRate, uint8_t numAgbBuffers)
    : ReverbEffect(intensity, streamRate, numAgbBuffers)
{
}

ReverbTest::~ReverbTest()
{
}

size_t ReverbTest::processInternal(sample *buffer, size_t numSamples)
{
    assert(numSamples > 0);
    std::vector<sample>& rbuf = reverbBuffer;
    size_t count = std::min(std::min(getBlocksPerBuffer() - bufferPos, getBlocksPerBuffer() - bufferPos2), numSamples);
    bool reset = false, reset2 = false;
    if (getBlocksPerBuffer() - bufferPos2 == count) {
        reset2 = true;
    }
    if (getBlocksPerBuffer() - bufferPos == count) {
        reset = true;
    }
    size_t c = count;
    do {
        const float g = 0.8f;
        float input_l = buffer->left;
        float input_r = buffer->right;

        float feedback_l = rbuf[bufferPos].left;
        float feedback_r = rbuf[bufferPos].right;

        float new_feedback_l = input_l + g * feedback_l;
        float new_feedback_r = input_r + g * feedback_r;

        float output_l = -g * new_feedback_l + feedback_l;
        float output_r = -g * new_feedback_r + feedback_r;

        buffer->left  = output_l;
        buffer->right = output_r;
        buffer++;

        rbuf[bufferPos].left  = -new_feedback_l;
        rbuf[bufferPos].right = -new_feedback_r;
        /*
           float in_delay_1_l = rbuf[bufferPos * 2], in_delay_1_r = rbuf[bufferPos * 2 + 1];
           float in_delay_2_l = rbuf[bufferPos2 * 2], in_delay_2_r = rbuf[bufferPos2 * 2 + 1];

           delay1HPcarryL = (in_delay_1_l + delay1HPcarryL - delay1HPprevL) * 0.95f;
           delay1HPcarryR = (in_delay_1_r + delay1HPcarryR - delay1HPprevR) * 0.95f;
           delay2HPcarryL = (in_delay_2_l + delay2HPcarryL - delay2HPprevL) * 0.95f;
           delay2HPcarryR = (in_delay_2_r + delay2HPcarryR - delay2HPprevR) * 0.95f;

           delay1HPprevL = in_delay_1_l;
           delay1HPprevR = in_delay_1_r;
           delay2HPprevL = in_delay_2_l;
           delay2HPprevR = in_delay_2_r;

           in_delay_1_l = delay1HPcarryL;
           in_delay_1_r = delay1HPcarryR;
           in_delay_2_l = delay2HPcarryL;
           in_delay_2_r = delay2HPcarryR;

           float r_left = in_delay_1_r * (4.0f / 8.0f) + in_delay_2_l * (4.0f / 8.0f);
           float r_right = -in_delay_1_l * (4.0f / 8.0f) - in_delay_2_r * (4.0f / 8.0f);

           rbuf[bufferPos * 2] = *buffer++ += r_left;
           rbuf[bufferPos * 2 + 1] = *buffer++ += r_right;
           */

        bufferPos++;
        bufferPos2++;
    } while (--c > 0);
    if (reset2) bufferPos2 = 0;
    if (reset) bufferPos = 0;
    return numSamples - count;
}
