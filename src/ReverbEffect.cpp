#include <algorithm>

#include "ReverbEffect.h"

using namespace agbplay;
using namespace std;

/*
 * public ReverbEffect
 */

ReverbEffect::ReverbEffect(int intensity, uint32_t streamRate, uint8_t numAgbBuffers)
    : reverbBuffer((streamRate / AGB_FPS) * N_CHANNELS * numAgbBuffers)
{
    this->intensity = 0.0f;
    if (intensity <= 0) {
        this->rtype = RevType::NONE;
    } else if (intensity <= 0x7F) {
        this->rtype = RevType::NORMAL;
        this->intensity = (float)intensity / 128.0f;
        // TODO check if this value should be devided by 256.0f
    } else {
        rtype = RevType::GS;
    }
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
        case RevType::NONE:
            break;
        case RevType::NORMAL:
            while (nBlocks > 0) {
                uint32_t left = processNormal(buffer, nBlocks);
                buffer += (nBlocks - left) * N_CHANNELS;
                nBlocks = left;
            }
            break;
        case RevType::GS:
            while (nBlocks > 0) {
                uint32_t left = processGS(buffer, nBlocks);
                buffer += (nBlocks - left) * N_CHANNELS;
                nBlocks = left;
            }
            break;
    }
}

uint32_t ReverbEffect::processNormal(float *buffer, uint32_t nBlocks)
{
    vector<float>& rbuf = reverbBuffer;
    uint32_t count;
    bool reset = false, reset2 = false;
    if (getBlocksPerBuffer() - bufferPos2 < nBlocks) {
        count = getBlocksPerBuffer() - bufferPos2;
        reset2 = true;
    } else if (getBlocksPerBuffer() - bufferPos < nBlocks) {
        count = getBlocksPerBuffer() - bufferPos;
        reset = true;
    } else {
        count = nBlocks;
    }
    uint32_t ipos = 0;
    for (uint32_t c = count; c > 0; c--) 
    {
        float rev = (rbuf[bufferPos * 2] + rbuf[bufferPos * 2 + 1] + 
                rbuf[bufferPos2 * 2] + rbuf[bufferPos2 * 2 + 1]) * intensity / 4.0f ;
        float oldl = buffer[ipos];
        buffer[ipos++] += rev;
        float oldr = buffer[ipos];
        buffer[ipos++] += rev;
        rbuf[bufferPos * 2] = oldl;
        rbuf[bufferPos++ * 2 + 1] = oldr;
    }
    if (reset2) bufferPos2 = 0;
    else if (reset) bufferPos = 0;
    return nBlocks - count;
}

uint32_t ReverbEffect::processGS(float *buffer, uint32_t nBlocks)
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
