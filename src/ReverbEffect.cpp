#include <algorithm>

#include "ReverbEffect.h"
#include "Debug.h"
#include "Util.h"

using namespace agbplay;
using namespace std;

/*
 * public ReverbEffect
 */

ReverbEffect::ReverbEffect(ReverbType rtype, uint8_t intensity, uint32_t streamRate, uint8_t numAgbBuffers)
    : reverbBuffer((streamRate / AGB_FPS) * N_CHANNELS * numAgbBuffers)
{
    this->intensity = (float)intensity / 128.0f;
    this->rtype = rtype;
    uint32_t bufferLen = streamRate / AGB_FPS;
    fill(reverbBuffer.begin(), reverbBuffer.end(), 0.0f);
    this->bufferPos = 0;
    this->bufferPos2 = bufferLen;
}

ReverbEffect::~ReverbEffect()
{
}

void ReverbEffect::ProcessData(float *buffer, uint32_t nBlocks)
{
    switch (rtype) {
        case ReverbType::NORMAL:
            while (nBlocks > 0) {
                uint32_t left = processNormal(buffer, nBlocks);
                buffer += (nBlocks - left) * N_CHANNELS;
                nBlocks = left;
            }
            break;
        case ReverbType::GS1:
            while (nBlocks > 0) {
                uint32_t left = processGS1(buffer, nBlocks);
                buffer += (nBlocks - left) * N_CHANNELS;
                nBlocks = left;
            }
            break;
        case ReverbType::GS2:
            while (nBlocks > 0) {
                uint32_t left = processGS2(buffer, nBlocks);
                buffer += (nBlocks - left) * N_CHANNELS;
                nBlocks = left;
            }
    }
}

uint32_t ReverbEffect::processNormal(float *buffer, uint32_t nBlocks)
{
    vector<float>& rbuf = reverbBuffer;
    uint32_t count;
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
    for (uint32_t c = count; c > 0; c--) 
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

uint32_t ReverbEffect::processGS1(float *buffer, uint32_t nBlocks)
{
    (void)buffer;
    (void)nBlocks;
    // TODO implement
    return 0;
}

uint32_t ReverbEffect::processGS2(float *buffer, uint32_t nBlocks)
{
    (void)buffer;
    (void)nBlocks;
    // TODO implement
    return 0;
}

uint32_t ReverbEffect::getBlocksPerBuffer()
{
    return uint32_t(reverbBuffer.size() / N_CHANNELS);
}
