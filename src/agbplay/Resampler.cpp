#include <boost/math/special_functions/sinc.hpp>

#include "Resampler.h"
#include "Util.h"
#include "Debug.h"

Resampler::~Resampler()
{
}

NearestResampler::NearestResampler()
{
}

NearestResampler::~NearestResampler()
{
}

void NearestResampler::Reset()
{
    fetchBuffer.clear();
    phase = 0.0f;
}

bool NearestResampler::Process(std::span<float> buffer, float phaseInc, const FetchCallback &fetchCallback)
{
    if (buffer.size() == 0)
        return true;

    phaseInc = std::max(phaseInc, 0.0f);

    size_t samplesRequired = size_t(phase + phaseInc * static_cast<float>(buffer.size()));
    // be sure and fetch one more sample in case of odd rounding errors
    samplesRequired += 1;
    const bool continuePlayback = fetchCallback(fetchBuffer, samplesRequired);

    size_t fi = 0;
    for (size_t i = 0; i < buffer.size(); i++) {
        buffer[i] = fetchBuffer[fi];
        phase += phaseInc;
        size_t istep = static_cast<size_t>(phase);
        phase -= static_cast<float>(istep);
        fi += istep;
    }

    // remove first fi elements from the fetch buffer since they are no longer needed
    fetchBuffer.erase(fetchBuffer.begin(), fetchBuffer.begin() + fi);

    return continuePlayback;
}

LinearResampler::LinearResampler()
{
    Reset();
}

LinearResampler::~LinearResampler()
{
}

void LinearResampler::Reset()
{
    fetchBuffer.clear();
    phase = 0.0f;
}

bool LinearResampler::Process(std::span<float> buffer, float phaseInc, const FetchCallback &fetchCallback)
{
    if (buffer.size() == 0)
        return true;

    phaseInc = std::max(phaseInc, 0.0f);

    size_t samplesRequired = static_cast<size_t>(
            phase + phaseInc * static_cast<float>(buffer.size()));
    // be sure and fetch one more sample in case of odd rounding errors
    samplesRequired += 1;
    // fetch one more for linear interpolation
    samplesRequired += 1;
    const bool continuePlayback = fetchCallback(fetchBuffer, samplesRequired);

    size_t fi = 0;
    for (size_t i = 0; i < buffer.size(); i++) {
        float a = fetchBuffer[fi];
        float b = fetchBuffer[fi+1];
        buffer[i] = a + phase * (b - a);
        phase += phaseInc;
        size_t istep = static_cast<size_t>(phase);
        phase -= static_cast<float>(istep);
        fi += istep;
    }

    // remove first fi elements from the fetch buffer since they are no longer needed
    fetchBuffer.erase(fetchBuffer.begin(), fetchBuffer.begin() + fi);

    return continuePlayback;
}

//static float triangle(float t)
//{
//    if (t < -1.0f)
//        return 0.0f;
//    else if (t < 0.0f)
//        return t + 1.0f;
//    else if (t < 1.0f)
//        return 1.0f - t;
//    else
//        return 0.0f;
//}

#define SINC_WINDOW_SIZE 16
#define SINC_FILT_THRESH 0.85f

SincResampler::SincResampler()
{
    Reset();
}

SincResampler::~SincResampler()
{
}

void SincResampler::Reset()
{
    fetchBuffer.clear();
    fetchBuffer.resize(SINC_WINDOW_SIZE, 0.0f);
    phase = 0.0f;
}

bool SincResampler::Process(std::span<float> buffer, float phaseInc, const FetchCallback &fetchCallback)
{
    if (buffer.size()== 0)
        return true;

    phaseInc = std::max(phaseInc, 0.0f);

    size_t samplesRequired = static_cast<size_t>(
            phase + phaseInc * static_cast<float>(buffer.size()));
    // be sure and fetch one more sample in case of odd rounding errors
    samplesRequired += 1;
    // fetch a few more for complete windowed sinc interpolation
    samplesRequired += SINC_WINDOW_SIZE * 2;
    const bool continuePlayback = fetchCallback(fetchBuffer, samplesRequired);

    const float sincStep = phaseInc > SINC_FILT_THRESH ? SINC_FILT_THRESH / phaseInc : 1.00f;

    //_print_debug("phaseInc=%f sincStep=%f", phaseInc, sincStep);

    size_t fi = 0;
    for (size_t i = 0; i < buffer.size(); i++) {
        float sampleSum = 0.0f;
        float kernelSum = 0.0f;
        for (int wi = -SINC_WINDOW_SIZE + 1; wi <= SINC_WINDOW_SIZE; wi++) {
            float sincIndex = (float(wi) - phase) * sincStep;
            float windowIndex = float(wi) - phase;
            //_print_debug("wi=%zu phase=%f sincStep=%f sincIndex=%f", wi, phase, sincStep, sincIndex);
            float s = fast_sincf(sincIndex);
            float w = window_func(windowIndex);
            //float s = triangle(sincIndex);
            //float w = 1.0f;
            float kernel = s * w;
            sampleSum += kernel * fetchBuffer[fi + static_cast<size_t>(wi + SINC_WINDOW_SIZE) - 1];
            kernelSum += kernel;
            //_print_debug("s=%f w=%f fetchBuffer[fi + wi]=%f", s, w, fetchBuffer[fi + wi]);
        }
        //_print_debug("sum=%f", sum);
        phase += phaseInc;
        size_t istep = static_cast<size_t>(phase);
        phase -= static_cast<float>(istep);
        fi += istep;

        buffer[i] = sampleSum / kernelSum;
        //*outData++ = sampleSum;
        //_print_debug("kernel sum: %f\n", kernelSum);
    }
    // remove first fi elements from the fetch buffer since they are no longer needed
    fetchBuffer.erase(fetchBuffer.begin(), fetchBuffer.begin() + fi);

    return continuePlayback;
}

/*
 * fast trigonometric functions
 */

// anything higher than 256 LUT size seems to be indistinguishable
#define LUT_SIZE 256

static const std::vector<float> cos_lut = []() {
    std::vector<float> l(LUT_SIZE);
    for (size_t i = 0; i < l.size(); i++) {
        float index = float(i) * float(2.0 * M_PI / double(LUT_SIZE));
        l[i] = cosf(index);
    }
    l.shrink_to_fit();
    return l;
}();

static const std::vector<float> sinc_lut = []() {
    std::vector<float> l(LUT_SIZE+2);
    for (size_t i = 0; i < LUT_SIZE+1; i++) {
        float index = float(i) * float(SINC_WINDOW_SIZE * M_PI / double(LUT_SIZE));
        l[i] = boost::math::sinc_pi(index);
    }
    l[LUT_SIZE+1] = 0.0f;
    l.shrink_to_fit();
    return l;
}();

static const std::vector<float> win_lut = []() {
    // hann window (raised cosine)
    std::vector<float> l(LUT_SIZE+2);
    for (size_t i = 0; i < LUT_SIZE+1; i++) {
        float index = float(i) * float(M_PI / double(LUT_SIZE));
        l[i] = 0.5f + (0.5f * cosf(index));
    }
    l[LUT_SIZE+1] = 0.0f;
    l.shrink_to_fit();
    return l;
}();

/*
static const std::vector<float> win_lut = []() {
    // nuttall window, experimental, performs worse than hann ?
    std::vector<float> l(LUT_SIZE+2);
    for (size_t i = 0; i < LUT_SIZE+1; i++) {
        const double a0 = 0.355768;
        const double a1 = 0.487396;
        const double a2 = 0.144232;
        const double a3 = 0.012604;
        const double N = static_cast<double>(LUT_SIZE);
        const double pi_i_N = 0.5 * M_PI + static_cast<double>(i) * M_PI / (2.0 * N);
        const double coeff = a0 - a1 * cos(2.0 * pi_i_N) + a2 * cos(4.0 * pi_i_N) - a3 * cos(6.0 * pi_i_N);
        l[i] = static_cast<float>(coeff);
    }
    l[LUT_SIZE+1] = l[LUT_SIZE];
    l.shrink_to_fit();
    return l;
}();
*/

float SincResampler::fast_sinf(float t)
{
    return SincResampler::fast_cosf(t - float(M_PI / 2.0));
}

float SincResampler::fast_cosf(float t)
{
    t = std::abs(t);
    t *= float(double(LUT_SIZE) / (2.0 * M_PI));
    uint32_t left_index = static_cast<uint32_t>(t);
    float fraction = t - static_cast<float>(left_index);
    uint32_t right_index = (left_index + 1) % LUT_SIZE;
    left_index %= LUT_SIZE;
    return cos_lut[left_index] + fraction * (cos_lut[right_index] - cos_lut[left_index]);
}

float SincResampler::fast_sincf(float t)
{
    t = std::abs(t);
    assert(t <= SINC_WINDOW_SIZE);
    t *= float(double(LUT_SIZE) / double(SINC_WINDOW_SIZE));
    uint32_t left_index = static_cast<uint32_t>(t);
    float fraction = t - static_cast<float>(left_index);
    uint32_t right_index = left_index + 1;
    return sinc_lut[left_index] + fraction * (sinc_lut[right_index] - sinc_lut[left_index]);
}

float SincResampler::window_func(float t)
{
    assert(t >= -float(SINC_WINDOW_SIZE));
    assert(t <= +float(SINC_WINDOW_SIZE));
    //return 0.42659f - 0.49656f * cosf(2.0f * PI_F * t / float(SINC_WINDOW_SIZE - 1)) +
    //    0.076849f * cosf(4.0f * PI_F * t / float(SINC_WINDOW_SIZE - 1));
    t = std::abs(t);
    t *= float(double(LUT_SIZE) / double(SINC_WINDOW_SIZE));
    uint32_t left_index = static_cast<uint32_t>(t);
    float fraction = t - static_cast<float>(left_index);
    uint32_t right_index = left_index + 1;
    return win_lut[left_index] + fraction * (win_lut[right_index] - win_lut[left_index]);
}

BlepResampler::BlepResampler()
{
    Reset();
}

BlepResampler::~BlepResampler()
{
}

void BlepResampler::Reset()
{
    fetchBuffer.clear();
    fetchBuffer.resize(SINC_WINDOW_SIZE, 0.0f);
    phase = 0.0f;
}

bool BlepResampler::Process(std::span<float> buffer, float phaseInc, const FetchCallback &fetchCallback)
{
    if (buffer.size() == 0)
        return true;

    phaseInc = std::max(phaseInc, 0.0f);

    size_t samplesRequired = static_cast<size_t>(
            phase + phaseInc * static_cast<float>(buffer.size()));
    // be sure and fetch one more sample in case of odd rounding errors
    samplesRequired += 1;
    // fetch a few more for complete windowed sinc interpolation
    samplesRequired += SINC_WINDOW_SIZE * 2;
    const bool continuePlayback = fetchCallback(fetchBuffer, samplesRequired);

    const float sincStep = SINC_FILT_THRESH / phaseInc;

    size_t fi = 0;
    for (size_t i = 0; i < buffer.size(); i++) {
        float sampleSum = 0.0f;
        float kernelSum = 0.0f;
        for (int wi = -SINC_WINDOW_SIZE + 1; wi <= SINC_WINDOW_SIZE; wi++) {
            float SiIndexLeft = (float(wi) - phase - 0.5f) * sincStep;
            float SiIndexRight = (float(wi) - phase + 0.5f) * sincStep;
            float sl = fast_Si(SiIndexLeft);
            float sr = fast_Si(SiIndexRight);
            float kernel = sr - sl;
            sampleSum += kernel * fetchBuffer[fi + static_cast<size_t>(wi + SINC_WINDOW_SIZE) - 1];
            kernelSum += kernel;
        }
        phase += phaseInc;
        size_t istep = static_cast<size_t>(phase);
        phase -= static_cast<float>(istep);
        fi += istep;

        buffer[i] = sampleSum / kernelSum;
        //*outData++ = sampleSum;
        //_print_debug("kernel sum: %f\n", kernelSum);
    }
    // remove first i elements from the fetch buffer since they are no longer needed
    fetchBuffer.erase(fetchBuffer.begin(), fetchBuffer.begin() + fi);

    return continuePlayback;
}

#define INTEGRAL_RESOLUTION 256

static const std::vector<float> Si_lut = []() {
    std::vector<float> l(LUT_SIZE+2);
    double acc = 0.0;
    double step_per_index = double(SINC_WINDOW_SIZE) / double(LUT_SIZE);
    double integration_inc = step_per_index / double(INTEGRAL_RESOLUTION);
    double index = 0.0;
    double prev_value = 1.0;

    for (size_t i = 0; i < LUT_SIZE+1; i++) {
        double convergence_level = 0.5 - 0.5 * cos(double(i) * M_PI / double(LUT_SIZE));
        double integral_level = 1.0 - convergence_level;
        l[i] = static_cast<float>(acc * integral_level + 0.5 * convergence_level);
        for (size_t j = 0; j < INTEGRAL_RESOLUTION; j++) {
            index += integration_inc;
            double new_value = boost::math::sinc_pi(M_PI * index);
            acc += (new_value + prev_value) * integration_inc * 0.5;
            prev_value = new_value;
        }
    }
    l[LUT_SIZE+1] = 0.5f;
    l.shrink_to_fit();
    return l;
}();

float BlepResampler::fast_Si(float t)
{
    float signed_t = t;
    t = std::abs(t);
    t = std::min(t, float(SINC_WINDOW_SIZE));
    t *= float(double(LUT_SIZE) / double(SINC_WINDOW_SIZE));
    uint32_t left_index = static_cast<uint32_t>(t);
    float fraction = t - static_cast<float>(left_index);
    uint32_t right_index = left_index + 1;
    float retval = Si_lut[left_index] + fraction * (Si_lut[right_index] - Si_lut[left_index]);
    return copysignf(retval, signed_t);
}

BlampResampler::BlampResampler()
{
    Reset();
}

BlampResampler::~BlampResampler()
{
}

void BlampResampler::Reset()
{
    fetchBuffer.clear();
    fetchBuffer.resize(SINC_WINDOW_SIZE, 0.0f);
    phase = 0.0f;
}

bool BlampResampler::Process(std::span<float> buffer, float phaseInc, const FetchCallback &fetchCallback)
{
    if (buffer.size() == 0)
        return true;

    phaseInc = std::max(phaseInc, 0.0f);

    size_t samplesRequired = static_cast<size_t>(
            phase + phaseInc * static_cast<float>(buffer.size()));
    // be sure and fetch one more sample in case of odd rounding errors
    samplesRequired += 1;
    // fetch a few more for complete windowed sinc interpolation
    samplesRequired += SINC_WINDOW_SIZE * 2;
    const bool continuePlayback = fetchCallback(fetchBuffer, samplesRequired);

    const float sincStep = SINC_FILT_THRESH / phaseInc;
    size_t fi = 0;
    for (size_t i = 0; i < buffer.size(); i++) {
        float sampleSum = 0.0f;
        float kernelSum = 0.0f;
        for (int wi = -SINC_WINDOW_SIZE + 1; wi <= SINC_WINDOW_SIZE; wi++) {
            float TiIndexLeft = (float(wi) - phase - 1.0f) * sincStep;
            float TiIndexMiddle = (float(wi) - phase) * sincStep;
            float TiIndexRight = (float(wi) - phase + 1.0f) * sincStep;
            float sl = fast_Ti(TiIndexLeft);
            float sm = fast_Ti(TiIndexMiddle);
            float sr = fast_Ti(TiIndexRight);
            float kernel = sr - 2.0f * sm + sl;
            sampleSum += kernel * fetchBuffer[fi + static_cast<size_t>(wi + SINC_WINDOW_SIZE) - 1];
            kernelSum += kernel;
        }
        phase += phaseInc;
        size_t istep = static_cast<size_t>(phase);
        phase -= static_cast<float>(istep);
        fi += istep;

        buffer[i] = sampleSum / kernelSum;
    }
    // remove first i elements from the fetch buffer since they are no longer needed
    fetchBuffer.erase(fetchBuffer.begin(), fetchBuffer.begin() + fi);

    return continuePlayback;
}

// I call "Ti" the integral of Si function. I don't know its proper name
static const std::vector<float> Ti_lut = []() {
    std::vector<float> l(LUT_SIZE+2);
    double acc = 0.0;
    double step_per_index = double(SINC_WINDOW_SIZE) / double(LUT_SIZE);
    double integration_inc = step_per_index / double(INTEGRAL_RESOLUTION);
    double index = 0.0;
    double prev_value = 1.0;

    for (size_t i = 0; i < LUT_SIZE+1; i++) {
        const double t = double(i) * step_per_index;
        const double convergence_value = t * 0.5;
        const double function_value = t * acc + cos(M_PI * t) / (M_PI * M_PI);
        const double interpolation_t = 0.5 - 0.5 * cos(double(i) * M_PI / double(LUT_SIZE));
        const double interpolated_value = function_value + interpolation_t * (convergence_value - function_value);
        l[i] = static_cast<float>(interpolated_value);

        for (size_t j = 0; j < INTEGRAL_RESOLUTION; j++) {
            index += integration_inc;
            double new_value = boost::math::sinc_pi(M_PI * index);
            acc += (new_value + prev_value) * integration_inc * 0.5;
            prev_value = new_value;
        }
    }
    l[LUT_SIZE+1] = static_cast<float>(((LUT_SIZE+1) * step_per_index) * 0.5);
    l.shrink_to_fit();
    return l;
}();

float BlampResampler::fast_Ti(float t)
{
    t = std::abs(t);
    float ct = t;
    ct = std::min(ct, float(SINC_WINDOW_SIZE));
    ct *= float(double(LUT_SIZE) / double(SINC_WINDOW_SIZE));
    uint32_t left_index = static_cast<uint32_t>(ct);
    float fraction = ct - static_cast<float>(left_index);
    uint32_t right_index = left_index + 1;
    float retval = Ti_lut[left_index] + fraction * (Ti_lut[right_index] - Ti_lut[left_index]);
    if (t > float(SINC_WINDOW_SIZE))
        return t * 0.5f;
    else
        return retval;
}
