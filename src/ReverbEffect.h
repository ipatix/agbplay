#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

#include "Types.h"

#define AGB_FPS 60
#define N_CHANNELS 2

namespace agbplay
{
    class ReverbEffect
    {
        public:
            ReverbEffect(uint8_t intesity, size_t streamRate, uint8_t numAgbBuffers);
            virtual ~ReverbEffect();
            void ProcessData(float *buffer, size_t nBlocks);
        protected:
            virtual size_t processInternal(float *buffer, size_t nBlocks);
            size_t getBlocksPerBuffer() const;
            float intensity;
            //size_t streamRate;
            std::vector<float> reverbBuffer;
            size_t bufferPos;
            size_t bufferPos2;
    };

    class ReverbGS1 : public ReverbEffect
    {
        public:
            ReverbGS1(uint8_t intensity, size_t streamRate, uint8_t numAgbBuffers);
            ~ReverbGS1() override;
        protected:
            size_t processInternal(float *buffer, size_t nBlocks) override;
            size_t getBlocksPerGsBuffer() const;
            std::vector<float> gsBuffer;
    };

    class ReverbGS2 : public ReverbEffect
    {
        public:
            ReverbGS2(uint8_t intesity, size_t streamRate, uint8_t numAgbBuffers,
                    float rPrimFac, float rSecFac);
            ~ReverbGS2() override;
        protected:
            size_t processInternal(float *buffer, size_t nBlocks) override;
            std::vector<float> gs2Buffer;
            size_t gs2Pos;
            float rPrimFac, rSecFac;
    };

    class ReverbTest : public ReverbEffect
    {
        public:
            ReverbTest(uint8_t intesity, size_t streamRate, uint8_t numAgbBuffers);
            ~ReverbTest() override;
        protected:
            size_t processInternal(float *buffer, size_t nBlocks) override;
    };

    class ReverbPS1 : public ReverbEffect
    {
        public:
            ReverbPS1(uint8_t intesity, size_t streamRate, uint8_t numAgbBuffers);
            ~ReverbPS1() override;
        protected:
            size_t processInternal(float *buffer, size_t nBlocks) override;

            std::vector<float> ps1buffer;

            // ps1 reverb variables
            size_t currAddr;
            size_t startAddr;

            int FB_SRC_A;
            int FB_SRC_B;
            float IIR_ALPHA;
            float ACC_COEF_A;
            float ACC_COEF_B;
            float ACC_COEF_C;
            float ACC_COEF_D;
            float IIR_COEF;
            float FB_ALPHA;
            float FB_ALPHA_S; // non register value
            float FB_X;
            int IIR_DEST_A0;
            int IIR_DEST_A1;
            int ACC_SRC_A0;
            int ACC_SRC_A1;
            int ACC_SRC_B0;
            int ACC_SRC_B1;
            int IIR_SRC_A0;
            int IIR_SRC_A1;
            int IIR_DEST_B0;
            int IIR_DEST_B1;
            int ACC_SRC_C0;
            int ACC_SRC_C1;
            int ACC_SRC_D0;
            int ACC_SRC_D1;
            int IIR_SRC_B1;
            int IIR_SRC_B0;
            int MIX_DEST_A0;
            int MIX_DEST_A1;
            int MIX_DEST_B0;
            int MIX_DEST_B1;
            float IN_COEF_L;
            float IN_COEF_R;
    };
}
