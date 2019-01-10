#include <boost/math/special_functions/sinc.hpp>

#include "Resampler.h"
#include "Util.h"
#include "Debug.h"

// FIXME the resamplers fetch data greedy from the input stream
// this might possibly cause the stream to drop prefetched samples

Resampler::Resampler()
    : phase(0.0f)
{
}

Resampler::Resampler(size_t initSize)
    : fetchBuffer(initSize, 0.0f), phase(0.0f)
{
}

Resampler::~Resampler()
{
}

NearestResampler::NearestResampler()
{
}

NearestResampler::~NearestResampler()
{
}

bool NearestResampler::Process(float *outData, size_t numBlocks, float phaseInc, res_data_fetch_cb cbPtr, void *cbdata)
{
    if (numBlocks == 0)
        return true;

    size_t samplesRequired = size_t(phase + phaseInc * static_cast<float>(numBlocks));
    // be sure and fetch one more sample in case of odd rounding errors
    samplesRequired += 1;
    bool result = cbPtr(fetchBuffer, samplesRequired, cbdata);

    int i = 0;
    do {
        float sample = fetchBuffer[i];
        phase += phaseInc;
        int istep = static_cast<int>(phase);
        phase -= static_cast<float>(istep);
        i += istep;

        *outData++ = sample;
    } while (--numBlocks > 0);

    // remove first i elements from the fetch buffer since they are no longer needed
    fetchBuffer.erase(fetchBuffer.begin(), fetchBuffer.begin() + i);

    return result;
}

LinearResampler::LinearResampler()
{
}

LinearResampler::~LinearResampler()
{
}

bool LinearResampler::Process(float *outData, size_t numBlocks, float phaseInc, res_data_fetch_cb cbPtr, void *cbdata)
{
    if (numBlocks == 0)
        return true;

    size_t samplesRequired = static_cast<size_t>(
            phase + phaseInc * static_cast<float>(numBlocks));
    // be sure and fetch one more sample in case of odd rounding errors
    samplesRequired += 1;
    // fetch one more for linear interpolation
    samplesRequired += 1;
    bool result = cbPtr(fetchBuffer, samplesRequired, cbdata);

    int i = 0;
    do {
        float a = fetchBuffer[i];
        float b = fetchBuffer[i+1];
        float sample = a + phase * (b - a);
        phase += phaseInc;
        int istep = static_cast<int>(phase);
        phase -= static_cast<float>(istep);
        i += istep;

        *outData++ = sample;
    } while (--numBlocks > 0);

    // remove first i elements from the fetch buffer since they are no longer needed
    fetchBuffer.erase(fetchBuffer.begin(), fetchBuffer.begin() + i);

    return result;
}

static float sinc(float t)
{
    return boost::math::sinc_pi(float(M_PI) * t);
}

static float triangle(float t)
{
    if (t < -1.0f)
        return 0.0f;
    else if (t < 0.0f)
        return t + 1.0f;
    else if (t < 1.0f)
        return 1.0f - t;
    else
        return 0.0f;
}

SincResampler::SincResampler()
    : Resampler::Resampler(WINDOW_SIZE / 2)
{
}

SincResampler::~SincResampler()
{
}

bool SincResampler::Process(float *outData, size_t numBlocks, float phaseInc, res_data_fetch_cb cbPtr, void *cbdata)
{
    if (numBlocks == 0)
        return true;

    size_t samplesRequired = static_cast<size_t>(
            phase + phaseInc * static_cast<float>(numBlocks));
    // be sure and fetch one more sample in case of odd rounding errors
    samplesRequired += 1;
    // fetch a few more for complete windowed sinc interpolation
    samplesRequired += WINDOW_SIZE - 1;
    bool result = cbPtr(fetchBuffer, samplesRequired, cbdata);

    float sincStep = std::min(1.0f / phaseInc, 1.0f);
    float sincNorm = 1.0f / sincStep;

    //_print_debug("sincStep=%f", sincStep);

    int i = 0;
    do {
        float sum = 0.0f;
        for (size_t wi = 1; wi < WINDOW_SIZE; wi++) {
            float sincIndex = -float(WINDOW_SIZE / 2) + float(wi) * sincStep - phase;
            //_print_debug("wi=%zu phase=%f sincStep=%f sincIndex=%f", wi, phase, sincStep, sincIndex);
            float s = sinc(sincIndex);
            float w = windowFunc(float(wi) - phase);
            //float s = triangle(sincIndex);
            //float w = 1.0f;
            sum += s * w * fetchBuffer[i + wi];
            //_print_debug("s=%f w=%f fetchBuffer[i + wi]=%f", s, w, fetchBuffer[i + wi]);
        }
        //_print_debug("sum=%f", sum);
        phase += phaseInc;
        int istep = static_cast<int>(phase);
        phase -= static_cast<float>(istep);
        i += istep;

        *outData++ = sum * sincNorm;
    } while (--numBlocks > 0);

    // remove first i elements from the fetch buffer since they are no longer needed
    fetchBuffer.erase(fetchBuffer.begin(), fetchBuffer.begin() + i);

    return result;
}

const size_t SincResampler::WINDOW_SIZE = 17;

float SincResampler::windowFunc(float t)
{
    if (t <= 0.0f)
        return 0.0f;
    if (t >= WINDOW_SIZE)
        return 0.0f;
    return 0.42659f - 0.49656f * cosf(2.0f * float(M_PI) * t / float(WINDOW_SIZE - 1)) +
        0.076849f * cosf(4.0f * float(M_PI) * t / float(WINDOW_SIZE - 1));
    //return 1.0f - (1.0f + cosf(t * 2.0f * float(M_PI) / float(WINDOW_SIZE))) * 0.5f;
}
