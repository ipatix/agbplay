#include <cmath>
#include <cassert>

#include "LoudnessCalculator.h"
#include "Constants.h"
#include "Util.h"

LoudnessCalculator::LoudnessCalculator(const float lowpassFreq)
    : lpAlpha(calcAlpha(lowpassFreq))
{
}

void LoudnessCalculator::CalcLoudness(const sample *audio, size_t numSamples)
{
    for (size_t i = 0; i < numSamples; i++) {
        float l = audio->left;
        float r = audio->right;
        audio++;
        assert(!std::isnan(l) && !std::isnan(r));
        assert(!std::isinf(l) && !std::isinf(r));
        l *= l;
        r *= r;
        avgVolLeftSq = avgVolLeftSq + lpAlpha * (l - avgVolLeftSq);
        avgVolRightSq = avgVolRightSq + lpAlpha * (r - avgVolRightSq);
    }

    static const float sqrt_2 = sqrtf(2.0f);

    volLeft = sqrtf(avgVolLeftSq) * sqrt_2;
    volRight = sqrtf(avgVolRightSq) * sqrt_2;
}

void LoudnessCalculator::GetLoudness(float& lVol, float& rVol)
{
    lVol = volLeft;
    rVol = volRight;
}

void LoudnessCalculator::Reset()
{
    avgVolLeftSq = 0.f;
    avgVolRightSq = 0.f;
    volLeft = 0.f;
    volRight = 0.f;
}

float LoudnessCalculator::calcAlpha(float lowpassFreq)
{
    float rc = 1.0f / (lowpassFreq * 2.0f * float(M_PI));
    float dt = 1.0f / float(STREAM_SAMPLERATE);
    return dt / (rc + dt);
}
