#pragma once

#include <cstddef>

class LoudnessCalculator
{
public:
    LoudnessCalculator(const float lowpassFreq);
    LoudnessCalculator(const LoudnessCalculator&) = delete;
    LoudnessCalculator(LoudnessCalculator&&) = default;
    LoudnessCalculator& operator=(const LoudnessCalculator&) = delete;

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
