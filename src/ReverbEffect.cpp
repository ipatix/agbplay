#include <algorithm>
#include <cassert>

#include "ReverbEffect.h"
#include "Debug.h"
#include "Util.h"

/*
 * public ReverbEffect
 */

ReverbEffect::ReverbEffect(uint8_t intensity, size_t streamRate, uint8_t numAgbBuffers)
    : reverbBuffer((streamRate / AGB_FPS) * N_CHANNELS * numAgbBuffers, 0.f)
{
    this->intensity = (float)intensity / 128.0f;
    size_t bufferLen = streamRate / AGB_FPS;
    bufferPos = 0;
    bufferPos2 = bufferLen;
}

ReverbEffect::~ReverbEffect()
{
}

void ReverbEffect::ProcessData(float *buffer, size_t nBlocks)
{
    while (nBlocks > 0)
    {
        size_t left = processInternal(buffer, nBlocks);
        buffer += (nBlocks - left) * N_CHANNELS;
        nBlocks = left;
    }
}

/*
 * protected ReverbEffect
 */

size_t ReverbEffect::getBlocksPerBuffer() const
{
    return reverbBuffer.size() / N_CHANNELS;
}

size_t ReverbEffect::processInternal(float *buffer, size_t nBlocks)
{
    assert(nBlocks > 0);
    std::vector<float>& rbuf = reverbBuffer;
    size_t count = std::min(std::min(getBlocksPerBuffer() - bufferPos2, getBlocksPerBuffer() - bufferPos), nBlocks);
    bool reset = false, reset2 = false;
    if (getBlocksPerBuffer() - bufferPos == count) {
        reset = true;
    }
    if (getBlocksPerBuffer() - bufferPos2 == count) {
        reset2 = true;
    } 
    size_t c = count;
    do {
        float rev = (rbuf[bufferPos * 2] + rbuf[bufferPos * 2 + 1] + 
                rbuf[bufferPos2 * 2] + rbuf[bufferPos2 * 2 + 1]) * intensity * (1.0f / 4.0f);
        rbuf[bufferPos * 2] = *buffer++ += rev;
        rbuf[bufferPos * 2 + 1] = *buffer++ += rev;
        bufferPos++;
        bufferPos2++;
    } while (--c > 0);
    if (reset2) bufferPos2 = 0;
    if (reset) bufferPos = 0;
    return nBlocks - count;
}

/*
 * ReverbGS1
 */

ReverbGS1::ReverbGS1(uint8_t intensity, size_t streamRate, uint8_t numAgbBuffers)
    : ReverbEffect(intensity, streamRate, numAgbBuffers), 
    gsBuffer((streamRate / AGB_FPS) * N_CHANNELS, 0.f)
{
    bufferPos2 = 0;
}

ReverbGS1::~ReverbGS1()
{
}

size_t ReverbGS1::getBlocksPerGsBuffer() const
{
    return gsBuffer.size() / N_CHANNELS;
}

size_t ReverbGS1::processInternal(float *buffer, size_t nBlocks)
{
    std::vector<float>& rbuf = reverbBuffer;
    const size_t bPerBuf = getBlocksPerBuffer();
    const size_t bPerGsBuf = getBlocksPerGsBuffer();
    size_t count = std::min(std::min(bPerBuf - bufferPos, bPerGsBuf - bufferPos2), nBlocks);
    bool reset = false, resetGS = false;

    if (count == bPerBuf - bufferPos)
        reset = true;
    if (count == bPerGsBuf - bufferPos2)
        resetGS = true;


    size_t c = count;
    do {
        float mixL = buffer[0] + gsBuffer[bufferPos2 * 2    ];
        float mixR = buffer[1] + gsBuffer[bufferPos2 * 2 + 1];

        float lA = rbuf[bufferPos * 2    ];
        float rA = rbuf[bufferPos * 2 + 1];

        buffer[0] = rbuf[bufferPos * 2    ] = mixL;
        buffer[1] = rbuf[bufferPos * 2 + 1] = mixR;

        float lRMix = 0.25f * mixL + 0.25f * rA;
        float rRMix = 0.25f * mixR + 0.25f * lA;

        gsBuffer[bufferPos2 * 2    ] = lRMix;
        gsBuffer[bufferPos2 * 2 + 1] = rRMix;

        buffer += 2;

        bufferPos++;
        bufferPos2++;
    } while (--c > 0);

    if (resetGS) bufferPos2 = 0;
    if (reset) bufferPos = 0;
    return nBlocks - count;
}

/*
 * ReverbGS2
 */

ReverbGS2::ReverbGS2(uint8_t intensity, size_t streamRate, uint8_t numAgbBuffers,
        float rPrimFac, float rSecFac)
    : ReverbEffect(intensity, streamRate, numAgbBuffers), 
    gs2Buffer(streamRate / AGB_FPS * N_CHANNELS, 0.f)
{
    // equivalent to the offset of -0xB0 samples for a 0x210 buffer size
    bufferPos2 = getBlocksPerBuffer() - (gs2Buffer.size() / N_CHANNELS / 3);
    gs2Pos = 0;
    this->rPrimFac = rPrimFac;
    this->rSecFac = rSecFac;
}

ReverbGS2::~ReverbGS2()
{
}

size_t ReverbGS2::processInternal(float *buffer, size_t nBlocks)
{
    assert(nBlocks > 0);
    std::vector<float>& rbuf = reverbBuffer;
    size_t count = std::min(
            std::min(getBlocksPerBuffer() - bufferPos2, getBlocksPerBuffer() - bufferPos), 
            std::min(nBlocks, gs2Buffer.size() / N_CHANNELS - gs2Pos)
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
        float mixL = buffer[0] + gs2Buffer[gs2Pos * 2    ];
        float mixR = buffer[1] + gs2Buffer[gs2Pos * 2 + 1];

        float lA = rbuf[bufferPos * 2    ];
        float rA = rbuf[bufferPos * 2 + 1];

        buffer[0] = rbuf[bufferPos * 2    ] = mixL;
        buffer[1] = rbuf[bufferPos * 2 + 1] = mixR;

        float lRMix = lA * rPrimFac + rA * rSecFac;
        float rRMix = rA * rPrimFac + lA * rSecFac;

        float lB = rbuf[bufferPos2 * 2 + 1] * 0.25f;
        float rB = mixR * 0.25f;

        gs2Buffer[gs2Pos * 2    ] = lRMix + lB;
        gs2Buffer[gs2Pos * 2 + 1] = rRMix + rB;

        buffer += 2;

        bufferPos++;
        bufferPos2++;
        gs2Pos++;
    } while (--c > 0);
    if (reset2) bufferPos2 = 0;
    if (reset) bufferPos = 0;
    if (resetgs2) gs2Pos = 0;
    return nBlocks - count;
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

size_t ReverbTest::processInternal(float *buffer, size_t nBlocks)
{
    assert(nBlocks > 0);
    std::vector<float>& rbuf = reverbBuffer;
    size_t count = std::min(std::min(getBlocksPerBuffer() - bufferPos, getBlocksPerBuffer() - bufferPos2), nBlocks);
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
        float input_l = buffer[0];
        float input_r = buffer[1];

        float feedback_l = rbuf[bufferPos * 2    ];
        float feedback_r = rbuf[bufferPos * 2 + 1];

        float new_feedback_l = input_l + g * feedback_l;
        float new_feedback_r = input_r + g * feedback_r;

        float output_l = -g * new_feedback_l + feedback_l;
        float output_r = -g * new_feedback_r + feedback_r;

        *buffer++ = output_l;
        *buffer++ = output_r;

        rbuf[bufferPos * 2    ] = -new_feedback_l;
        rbuf[bufferPos * 2 + 1] = -new_feedback_r;
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
    return nBlocks - count;
}
