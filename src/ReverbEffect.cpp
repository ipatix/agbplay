#include <algorithm>
#include <cassert>

#include "ReverbEffect.h"
#include "Debug.h"
#include "Util.h"

using namespace agbplay;
using namespace std;

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

size_t ReverbEffect::getBlocksPerBuffer()
{
    return reverbBuffer.size() / N_CHANNELS;
}

size_t ReverbEffect::processInternal(float *buffer, size_t nBlocks)
{
    vector<float>& rbuf = reverbBuffer;
    size_t count;
    bool reset = false, reset2 = false;
    if (getBlocksPerBuffer() - bufferPos2 <= nBlocks) {
        count = getBlocksPerBuffer() - bufferPos2;
        reset2 = true;
    } else if (getBlocksPerBuffer() - bufferPos <= nBlocks) {
        count = getBlocksPerBuffer() - bufferPos;
        reset = true;
    } else {
        count = nBlocks;
    }
    for (size_t c = count; c > 0; c--) 
    {
        float rev = (rbuf[bufferPos * 2] + rbuf[bufferPos * 2 + 1] + 
                rbuf[bufferPos2 * 2] + rbuf[bufferPos2 * 2 + 1]) * intensity * (1.0f / 4.0f);
        rbuf[bufferPos * 2] = *buffer++ += rev;
        rbuf[bufferPos * 2 + 1] = *buffer++ += rev;
        bufferPos++;
        bufferPos2++;
    }
    if (reset2) bufferPos2 = 0;
    else if (reset) bufferPos = 0;
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

size_t ReverbGS1::getBlocksPerGsBuffer()
{
    return gsBuffer.size() / N_CHANNELS;
}

size_t ReverbGS1::processInternal(float *buffer, size_t nBlocks)
{
    // FIXME experimental, just to mess around and not to implement the actual alogrithm
    vector<float>& rbuf = reverbBuffer;
    size_t count;
    bool reset = false, resetGS = false;

    const size_t bPerBuf = getBlocksPerBuffer();
    const size_t bPerGsBuf = getBlocksPerGsBuffer();

    
    if (min(bPerBuf - bufferPos, bPerGsBuf - bufferPos2) <= nBlocks)
    {
        if (bPerBuf - bufferPos < bPerGsBuf - bufferPos2)
        {
            reset = true;
            count = bPerBuf - bufferPos;
        }
        else if (bPerBuf - bufferPos == bPerGsBuf - bufferPos2)
        {
            reset = true;
            resetGS = true;
            count = bPerBuf - bufferPos;
        }
        else
        {
            resetGS = true;
            count = bPerGsBuf - bufferPos2;
        }
    }
    else
    {
        count = nBlocks;
    }

    assert(count % 4 == 0);
    for (size_t c = count; c > 0; c--)
    {
        float rev_l = gsBuffer[bufferPos2 * 2];
        float rev_r = gsBuffer[bufferPos2 * 2 + 1];
        float in_l = buffer[0];
        float in_r = buffer[1];
        float long_rev_l = rbuf[bufferPos * 2];
        float long_rev_r = rbuf[bufferPos * 2 + 1];

        float new_l = in_l + (1.f / 128.f) * rev_l + (1.f / 64.f) * long_rev_l;
        float new_r = in_r + (1.f / 128.f) * rev_r + (1.f / 64.f) * long_rev_r;
        gsBuffer[bufferPos2 * 2] = new_l;
        gsBuffer[bufferPos2 * 2 + 1] = new_r;

        *buffer++ += rbuf[bufferPos * 2];
        *buffer++ += rbuf[bufferPos * 2 + 1];

        rbuf[bufferPos * 2] = new_l;
        rbuf[bufferPos * 2 + 1] = new_r;

        bufferPos++;
        bufferPos2++;
    }

    if (resetGS) bufferPos2 = 0;
    if (reset) bufferPos = 0;
    return nBlocks - count;
}

/*
 * ReverbGS2
 */

ReverbGS2::ReverbGS2(uint8_t intensity, size_t streamRate, uint8_t numAgbBuffers)
    : ReverbEffect(intensity, streamRate, numAgbBuffers)
{
}

ReverbGS2::~ReverbGS2()
{
}

size_t ReverbGS2::processInternal(float *buffer, size_t nBlocks)
{
    vector<float>& rbuf = reverbBuffer;
    size_t count;
    bool reset = false, reset2 = false;
    if (getBlocksPerBuffer() - bufferPos2 <= nBlocks) {
        count = getBlocksPerBuffer() - bufferPos2;
        reset2 = true;
    } else if (getBlocksPerBuffer() - bufferPos <= nBlocks) {
        count = getBlocksPerBuffer() - bufferPos;
        reset = true;
    } else {
        count = nBlocks;
    }
    for (size_t c = count; c > 0; c--) 
    {
        (void)rbuf;
        (void)buffer;
        bufferPos++;
        bufferPos2++;
    }
    if (reset2) bufferPos2 = 0;
    else if (reset) bufferPos = 0;
    return nBlocks - count;
}

/*
 * protected ReverbTest
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
    vector<float>& rbuf = reverbBuffer;
    size_t count;
    bool reset = false, reset2 = false;
    if (getBlocksPerBuffer() - bufferPos2 <= nBlocks) {
        count = getBlocksPerBuffer() - bufferPos2;
        reset2 = true;
    } else if (getBlocksPerBuffer() - bufferPos <= nBlocks) {
        count = getBlocksPerBuffer() - bufferPos;
        reset = true;
    } else {
        count = nBlocks;
    }
    for (size_t c = count; c > 0; c--) 
    {
        (void)rbuf;
        (void)buffer;
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
    }
    if (reset2) bufferPos2 = 0;
    else if (reset) bufferPos = 0;
    return nBlocks - count;
}
