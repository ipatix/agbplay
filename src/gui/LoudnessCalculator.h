#pragma once

#include <cstddef>

#include "Types.h"

class LoudnessCalculator
{
public:
    LoudnessCalculator(const float lowpassFreq);
    LoudnessCalculator(const LoudnessCalculator&) = delete;
    LoudnessCalculator(LoudnessCalculator&&) = default;
    LoudnessCalculator& operator=(const LoudnessCalculator&) = delete;

    void CalcLoudness(const sample *audio, size_t numSamples);
    void GetLoudness(float& lVol, float& rVol);
    void Reset();
private:
    static float calcAlpha(float lowpassFreq);

    float lpAlpha;
    float avgVolLeftSq = 0.0f;
    float avgVolRightSq = 0.0f;

    float volLeft = 0.0f;
    float volRight = 0.0f;
};
