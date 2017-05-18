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

size_t ReverbEffect::getBlocksPerBuffer() const
{
    return reverbBuffer.size() / N_CHANNELS;
}

size_t ReverbEffect::processInternal(float *buffer, size_t nBlocks)
{
    assert(nBlocks > 0);
    vector<float>& rbuf = reverbBuffer;
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
    vector<float>& rbuf = reverbBuffer;
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
    vector<float>& rbuf = reverbBuffer;
    size_t count = std::min(
            std::min(getBlocksPerBuffer() - bufferPos2, getBlocksPerBuffer() - bufferPos), 
            std::min(nBlocks, gs2Buffer.size() / N_CHANNELS - gs2Pos)
            );
    bool reset = false, reset2 = false, resetgs2 = false;

    if (getBlocksPerBuffer() - bufferPos2 <= count) {
        reset2 = true;
    } 
    if (getBlocksPerBuffer() - bufferPos <= count) {
        reset = true;
    }
    if ((gs2Buffer.size() / 2) - gs2Pos <= count) {
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
    vector<float>& rbuf = reverbBuffer;
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
    } while (--c > 0);
    if (reset2) bufferPos2 = 0;
    if (reset) bufferPos = 0;
    return nBlocks - count;
}

ReverbPS1::ReverbPS1(uint8_t intesity, size_t streamRate, uint8_t numAgbBuffers) 
    : ReverbEffect(intesity, streamRate, numAgbBuffers)
{
    currAddr = 0x0;
    startAddr = 0x0;

    // test values

    FB_SRC_A    = 0x01A5;
    FB_SRC_B    = 0x0139;
    IIR_ALPHA   = float(+0x6000) / float(0x8000);
    ACC_COEF_A  = float(+0x5000) / float(0x8000);
    ACC_COEF_B  = float(+0x4C00) / float(0x8000);
    ACC_COEF_C  = float(-0x4800) / float(0x8000); //0xB800;
    ACC_COEF_D  = float(-0x4400) / float(0x8000); //0xBC00;
    IIR_COEF    = float(-0x4000) / float(0x8000); //0xC000;
    FB_ALPHA    = float(+0x6000) / float(0x8000);
    FB_ALPHA_S  = FB_ALPHA - 1.0f;
    FB_X        = float(+0x5C00) / float(0x8000);
    IIR_DEST_A0 = 0x15BA;
    IIR_DEST_A1 = 0x11BB;
    ACC_SRC_A0  = 0x14C2;
    ACC_SRC_A1  = 0x10BD;
    ACC_SRC_B0  = 0x11BC;
    ACC_SRC_B1  = 0x0DC1;
    IIR_SRC_A0  = 0x11C0;
    IIR_SRC_A1  = 0x0DC3;
    IIR_DEST_B0 = 0x0DC0;
    IIR_DEST_B1 = 0x09C1;
    ACC_SRC_C0  = 0x0BC4;
    ACC_SRC_C1  = 0x07C1;
    ACC_SRC_D0  = 0x0A00;
    ACC_SRC_D1  = 0x06CD;
    IIR_SRC_B1  = 0x09C2;
    IIR_SRC_B0  = 0x05C1;
    MIX_DEST_A0 = 0x05C0;
    MIX_DEST_A1 = 0x041A;
    MIX_DEST_B0 = 0x0274;
    MIX_DEST_B1 = 0x013A;
    IN_COEF_L   = float(-0x8000) / float(0x8000); // 0x8000
    IN_COEF_R   = float(-0x8000) / float(0x8000); // 0x8000

    ps1buffer.resize(0x40000);
}

ReverbPS1::~ReverbPS1()
{
}

size_t ReverbPS1::processInternal(float *buffer, size_t nBlocks) 
{
    auto rvbIn = [this](int index){
        int sz = int(ps1buffer.size());
        return size_t((((index + (int)this->currAddr) % sz) + sz) % sz);
    };

    size_t c = nBlocks;

    // algorithm below (C) 2002 by Pete Bernert from the audacious PSF plugin

    do {
        float input_l = buffer[0];
        float input_r = buffer[1];
        float IIR_INPUT_A0 = ps1buffer[rvbIn(IIR_SRC_A0 * 4)] * IIR_COEF + input_l * IN_COEF_L;
        float IIR_INPUT_A1 = ps1buffer[rvbIn(IIR_SRC_A1 * 4)] * IIR_COEF + input_r * IN_COEF_R;
        float IIR_INPUT_B0 = ps1buffer[rvbIn(IIR_SRC_B0 * 4)] * IIR_COEF + input_l * IN_COEF_L;
        float IIR_INPUT_B1 = ps1buffer[rvbIn(IIR_SRC_B1 * 4)] * IIR_COEF + input_r * IN_COEF_R;
        float IIR_A0 = IIR_INPUT_A0 * IIR_ALPHA + ps1buffer[rvbIn(IIR_DEST_A0 * 4)] * (1.0f - IIR_ALPHA);
        float IIR_A1 = IIR_INPUT_A1 * IIR_ALPHA + ps1buffer[rvbIn(IIR_DEST_A1 * 4)] * (1.0f - IIR_ALPHA);
        float IIR_B0 = IIR_INPUT_B0 * IIR_ALPHA + ps1buffer[rvbIn(IIR_DEST_B0 * 4)] * (1.0f - IIR_ALPHA);
        float IIR_B1 = IIR_INPUT_B1 * IIR_ALPHA + ps1buffer[rvbIn(IIR_DEST_B1 * 4)] * (1.0f - IIR_ALPHA);
    
        ps1buffer[rvbIn(IIR_DEST_A0 * 4 + 1)] = IIR_A0;
        ps1buffer[rvbIn(IIR_DEST_A1 * 4 + 1)] = IIR_A1;
        ps1buffer[rvbIn(IIR_DEST_B0 * 4 + 1)] = IIR_B0;
        ps1buffer[rvbIn(IIR_DEST_B1 * 4 + 1)] = IIR_B1;
    
        float ACC0 = 
            ps1buffer[rvbIn(ACC_SRC_A0 * 4)] * ACC_COEF_A +
            ps1buffer[rvbIn(ACC_SRC_B0 * 4)] * ACC_COEF_B +   
            ps1buffer[rvbIn(ACC_SRC_C0 * 4)] * ACC_COEF_C +   
            ps1buffer[rvbIn(ACC_SRC_D0 * 4)] * ACC_COEF_D;    
        float ACC1 = 
            ps1buffer[rvbIn(ACC_SRC_A1 * 4)] * ACC_COEF_A +
            ps1buffer[rvbIn(ACC_SRC_B1 * 4)] * ACC_COEF_B +   
            ps1buffer[rvbIn(ACC_SRC_C1 * 4)] * ACC_COEF_C +   
            ps1buffer[rvbIn(ACC_SRC_D1 * 4)] * ACC_COEF_D;    

        float FB_A0 = ps1buffer[rvbIn((MIX_DEST_A0 - FB_SRC_A) * 4)];
        float FB_A1 = ps1buffer[rvbIn((MIX_DEST_A1 - FB_SRC_A) * 4)];
        float FB_B0 = ps1buffer[rvbIn((MIX_DEST_B0 - FB_SRC_B) * 4)];
        float FB_B1 = ps1buffer[rvbIn((MIX_DEST_B1 - FB_SRC_B) * 4)];

        ps1buffer[rvbIn(MIX_DEST_A0 * 4)] = ACC0 - FB_A0 * FB_ALPHA;
        ps1buffer[rvbIn(MIX_DEST_A1 * 4)] = ACC1 - FB_A1 * FB_ALPHA;

        ps1buffer[rvbIn(MIX_DEST_B0 * 4)] = FB_ALPHA * ACC0 - FB_A0 - FB_ALPHA_S - FB_B0 * FB_X;
        ps1buffer[rvbIn(MIX_DEST_B1 * 4)] = FB_ALPHA * ACC0 - FB_A1 - FB_ALPHA_S - FB_B1 * FB_X;
            
        float out_l = ps1buffer[rvbIn(MIX_DEST_A0 * 4)] + ps1buffer[rvbIn(MIX_DEST_B0 * 4)];
        float out_r = ps1buffer[rvbIn(MIX_DEST_A1 * 4)] + ps1buffer[rvbIn(MIX_DEST_B1 * 4)];

        *buffer++ = out_l / 3.0f;
        *buffer++ = out_r / 3.0f;

        currAddr++;
        if (currAddr >= ps1buffer.size())
            currAddr = startAddr;
    } while (--c > 0);
    return 0;
}
