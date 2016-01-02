#pragma once

#include <cstdint>
#include <vector>

#define AGB_FPS 60
#define N_CHANNELS 2

namespace agbplay
{
    class ReverbEffect
    {
        public:
            ReverbEffect(int intesity, uint32_t streamRate, uint8_t numAgbBuffers);
            ~ReverbEffect();
            void ProcessData(float *buffer, uint32_t nBlocks);
        private:
            uint32_t processNormal(float *buffer, uint32_t nBlocks);
            uint32_t processGS(float *buffer, uint32_t nBlocks);
            uint32_t getBlocksPerBuffer();
            enum class RevType { NONE, NORMAL, GS } rtype;
            float intensity;
            //uint32_t streamRate;
            std::vector<float> *reverbBuffer;
            uint32_t bufferPos;
            uint32_t bufferPos2;
    };
}
