#pragma once

#include <cstdint>

namespace agbplay
{
    class ReverbEffect
    {
        public:
            ReverbEffect(int intesity, uint32_t streamRate, uint32_t agbRate, uint8_t numAgbBuffers);
            ~ReverbEffect();
            void ProcessData(float *buffer, uint32_t amount);
        private:
            enum class RevType { NONE, NORMAL, GS } rtype;
            int intensity;
            uint32_t streamRate;
    };
}
