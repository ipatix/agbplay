#pragma once

#include <cstddef>

namespace agbplay
{
    class LoudnessCalculator
    {
        public:
            LoudnessCalculator(const float lowpassFreq);

            void CalcLoudness(const float *audio, const size_t nBlocks);
            void GetLoudness(float& lVol, float& rVol);
            void Reset();
        private:
            float lpAlpha;
            float avgVolLeftSq;
            float avgVolRightSq;

            float volLeft;
            float volRight;
    };
}
