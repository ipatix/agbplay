#include "Resampler.hpp"

#include "Debug.hpp"
#include "ResamplerAVX2.hpp"
#include "Util.hpp"

#include <boost/math/special_functions/sinc.hpp>

const bool AVX2_SUPPORTED = []() {
#if defined(__x86_64__) || defined(i386) || defined(__i386__) || defined(__386)
    __builtin_cpu_init();
    if (__builtin_cpu_supports("avx2"))
        return true;
    else
        return false;
#elif defined(_M_X64) || defined(_M_IX86)
    static_assert(false, "AVX2 detection in MSVC is not yet implemented");
#else
    return false;
#endif
}();

std::unique_ptr<Resampler> Resampler::MakeResampler(ResamplerType t)
{
    switch (t) {
    case ResamplerType::NEAREST:
        return std::make_unique<NearestResampler>();
    case ResamplerType::LINEAR:
        return std::make_unique<LinearResampler>();
    case ResamplerType::SINC:
        if (AVX2_SUPPORTED)
            return std::make_unique<SincResamplerAVX2>();
        else
            return std::make_unique<SincResampler>();
    case ResamplerType::BLEP:
        if (AVX2_SUPPORTED)
            return std::make_unique<BlepResamplerAVX2>();
        else
            return std::make_unique<BlepResampler>();
    case ResamplerType::BLAMP:
        if (AVX2_SUPPORTED)
            return std::make_unique<BlampResamplerAVX2>();
        else
            return std::make_unique<BlampResampler>();
    }
    throw std::logic_error("MakeResampler: Trying to to instantiate resampler for invalid enum value");
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

    size_t samplesRequired = static_cast<size_t>(phase + phaseInc * static_cast<float>(buffer.size()));
    // be sure and fetch one more sample in case of odd rounding errors
    samplesRequired += 1;
    // fetch one more for linear interpolation
    samplesRequired += 1;
    const bool continuePlayback = fetchCallback(fetchBuffer, samplesRequired);

    size_t fi = 0;
    for (size_t i = 0; i < buffer.size(); i++) {
        float a = fetchBuffer[fi];
        float b = fetchBuffer[fi + 1];
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
    fetchBuffer.resize(INTERP_FILTER_SIZE, 0.0f);
    phase = 0.0f;
}

bool SincResampler::Process(std::span<float> buffer, float phaseInc, const FetchCallback &fetchCallback)
{
    if (buffer.size() == 0)
        return true;

    phaseInc = std::max(phaseInc, 0.0f);

    size_t samplesRequired = static_cast<size_t>(phase + phaseInc * static_cast<float>(buffer.size()));
    // be sure and fetch one more sample in case of odd rounding errors
    samplesRequired += 1;
    // fetch a few more for complete windowed sinc interpolation
    samplesRequired += INTERP_FILTER_SIZE * 2;
    const bool continuePlayback = fetchCallback(fetchBuffer, samplesRequired);
    const float sincStep = phaseInc > INTERP_FILTER_CUTOFF_FREQ ? INTERP_FILTER_CUTOFF_FREQ / phaseInc : 1.00f;

    //_print_debug("phaseInc=%f sincStep=%f", phaseInc, sincStep);

    size_t fi = 0;
    for (size_t i = 0; i < buffer.size(); i++) {
        float sampleSum = 0.0f;
        float kernelSum = 0.0f;

        for (int wi = -INTERP_FILTER_SIZE + 1; wi <= INTERP_FILTER_SIZE; wi++) {
            float sincIndex = (float(wi) - phase) * sincStep;
            float windowIndex = float(wi) - phase;
            //_print_debug("wi=%zu phase=%f sincStep=%f sincIndex=%f", wi, phase, sincStep, sincIndex);
            float s = fast_sincf(sincIndex);
            float w = window_func(windowIndex);
            // float s = triangle(sincIndex);
            // float w = 1.0f;
            float kernel = s * w;
            sampleSum += kernel * fetchBuffer[fi + static_cast<size_t>(wi + INTERP_FILTER_SIZE) - 1];
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

const std::array<float, Resampler::INTERP_FILTER_LUT_SIZE> SincResampler::cosLut = []() {
    std::array<float, INTERP_FILTER_LUT_SIZE> l;
    for (size_t i = 0; i < l.size(); i++) {
        float index = float(i) * float(2.0 * M_PI / double(INTERP_FILTER_LUT_SIZE));
        l[i] = std::cos(index);
    }
    return l;
}();

const std::array<float, Resampler::INTERP_FILTER_LUT_SIZE + 2> SincResampler::sincLut = []() {
    std::array<float, INTERP_FILTER_LUT_SIZE + 2> l;
    for (size_t i = 0; i < INTERP_FILTER_LUT_SIZE + 1; i++) {
        float index = float(i) * float(INTERP_FILTER_SIZE * M_PI / double(INTERP_FILTER_LUT_SIZE));
        l[i] = boost::math::sinc_pi(index);
    }
    l[INTERP_FILTER_LUT_SIZE + 1] = 0.0f;
    return l;
}();

const std::array<float, Resampler::INTERP_FILTER_LUT_SIZE + 2> SincResampler::winLut = []() {
    // hann window (raised cosine)
    std::array<float, INTERP_FILTER_LUT_SIZE + 2> l;
    for (size_t i = 0; i < INTERP_FILTER_LUT_SIZE + 1; i++) {
        float index = float(i) * float(M_PI / double(INTERP_FILTER_LUT_SIZE));
        l[i] = 0.5f + (0.5f * cosf(index));
    }
    l[INTERP_FILTER_LUT_SIZE + 1] = 0.0f;
    return l;
}();

/*
static const std::vector<float> win_lut = []() {
    // nuttall window, experimental, performs worse than hann ?
    std::vector<float> l(INTERP_FILTER_LUT_SIZE+2);
    for (size_t i = 0; i < INTERP_FILTER_LUT_SIZE+1; i++) {
        const double a0 = 0.355768;
        const double a1 = 0.487396;
        const double a2 = 0.144232;
        const double a3 = 0.012604;
        const double N = static_cast<double>(INTERP_FILTER_LUT_SIZE);
        const double pi_i_N = 0.5 * M_PI + static_cast<double>(i) * M_PI / (2.0 * N);
        const double coeff = a0 - a1 * cos(2.0 * pi_i_N) + a2 * cos(4.0 * pi_i_N) - a3 * cos(6.0 * pi_i_N);
        l[i] = static_cast<float>(coeff);
    }
    l[INTERP_FILTER_LUT_SIZE+1] = l[INTERP_FILTER_LUT_SIZE];
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
    t *= float(double(INTERP_FILTER_LUT_SIZE) / (2.0 * M_PI));
    uint32_t left_index = static_cast<uint32_t>(t);
    float fraction = t - static_cast<float>(left_index);
    uint32_t right_index = (left_index + 1) % INTERP_FILTER_LUT_SIZE;
    left_index %= INTERP_FILTER_LUT_SIZE;
    return cosLut[left_index] + fraction * (cosLut[right_index] - cosLut[left_index]);
}

float SincResampler::fast_sincf(float t)
{
    t = std::abs(t);
    // assert(t <= INTERP_FILTER_SIZE);
    t *= float(double(INTERP_FILTER_LUT_SIZE) / double(INTERP_FILTER_SIZE));
    uint32_t left_index = static_cast<uint32_t>(t);
    float fraction = t - static_cast<float>(left_index);
    uint32_t right_index = left_index + 1;
    return sincLut[left_index] + fraction * (sincLut[right_index] - sincLut[left_index]);
}

float SincResampler::window_func(float t)
{
    // assert(t >= -float(INTERP_FILTER_SIZE));
    // assert(t <= +float(INTERP_FILTER_SIZE));
    // return 0.42659f - 0.49656f * cosf(2.0f * PI_F * t / float(INTERP_FILTER_SIZE - 1)) +
    //     0.076849f * cosf(4.0f * PI_F * t / float(INTERP_FILTER_SIZE - 1));
    t = std::abs(t);
    t *= float(double(INTERP_FILTER_LUT_SIZE) / double(INTERP_FILTER_SIZE));
    uint32_t left_index = static_cast<uint32_t>(t);
    float fraction = t - static_cast<float>(left_index);
    uint32_t right_index = left_index + 1;
    return winLut[left_index] + fraction * (winLut[right_index] - winLut[left_index]);
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
    fetchBuffer.resize(INTERP_FILTER_SIZE, 0.0f);
    phase = 0.0f;
}

bool BlepResampler::Process(std::span<float> buffer, float phaseInc, const FetchCallback &fetchCallback)
{
    if (buffer.size() == 0)
        return true;

    phaseInc = std::max(phaseInc, 0.0f);

    size_t samplesRequired = static_cast<size_t>(phase + phaseInc * static_cast<float>(buffer.size()));
    // be sure and fetch one more sample in case of odd rounding errors
    samplesRequired += 1;
    // fetch a few more for complete windowed sinc interpolation
    samplesRequired += INTERP_FILTER_SIZE * 2;
    const bool continuePlayback = fetchCallback(fetchBuffer, samplesRequired);
    const float sincStep = INTERP_FILTER_CUTOFF_FREQ / phaseInc;

    size_t fi = 0;
    for (size_t i = 0; i < buffer.size(); i++) {
        float sampleSum = 0.0f;
        float kernelSum = 0.0f;

        for (int wi = -INTERP_FILTER_SIZE + 1; wi <= INTERP_FILTER_SIZE; wi++) {
            float SiIndexLeft = (float(wi) - phase - 0.5f) * sincStep;
            float SiIndexRight = (float(wi) - phase + 0.5f) * sincStep;
            float sl = fast_Si(SiIndexLeft);
            float sr = fast_Si(SiIndexRight);
            float kernel = sr - sl;
            sampleSum += kernel * fetchBuffer[fi + static_cast<size_t>(wi + INTERP_FILTER_SIZE) - 1];
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

const std::array<float, Resampler::INTERP_FILTER_LUT_SIZE + 2> BlepResampler::SiLut = []() {
    std::array<float, INTERP_FILTER_LUT_SIZE + 2> l;
    double acc = 0.0;
    double step_per_index = double(INTERP_FILTER_SIZE) / double(INTERP_FILTER_LUT_SIZE);
    double integration_inc = step_per_index / double(INTEGRAL_RESOLUTION);
    double index = 0.0;
    double prev_value = 1.0;

    for (size_t i = 0; i < INTERP_FILTER_LUT_SIZE + 1; i++) {
        double convergence_level = 0.5 - 0.5 * cos(double(i) * M_PI / double(INTERP_FILTER_LUT_SIZE));
        double integral_level = 1.0 - convergence_level;
        l[i] = static_cast<float>(acc * integral_level + 0.5 * convergence_level);
        for (size_t j = 0; j < INTEGRAL_RESOLUTION; j++) {
            index += integration_inc;
            double new_value = boost::math::sinc_pi(M_PI * index);
            acc += (new_value + prev_value) * integration_inc * 0.5;
            prev_value = new_value;
        }
    }
    l[INTERP_FILTER_LUT_SIZE + 1] = 0.5f;
    return l;
}();

float BlepResampler::fast_Si(float t)
{
    float signed_t = t;
    t = std::abs(t);
    t = std::min(t, float(INTERP_FILTER_SIZE));
    t *= float(double(INTERP_FILTER_LUT_SIZE) / double(INTERP_FILTER_SIZE));
    uint32_t left_index = static_cast<uint32_t>(t);
    float fraction = t - static_cast<float>(left_index);
    uint32_t right_index = left_index + 1;
    float retval = SiLut[left_index] + fraction * (SiLut[right_index] - SiLut[left_index]);
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
    fetchBuffer.resize(INTERP_FILTER_SIZE, 0.0f);
    phase = 0.0f;
}

bool BlampResampler::Process(std::span<float> buffer, float phaseInc, const FetchCallback &fetchCallback)
{
    if (buffer.size() == 0)
        return true;

    phaseInc = std::max(phaseInc, 0.0f);

    size_t samplesRequired = static_cast<size_t>(phase + phaseInc * static_cast<float>(buffer.size()));
    // be sure and fetch one more sample in case of odd rounding errors
    samplesRequired += 1;
    // fetch a few more for complete windowed sinc interpolation
    samplesRequired += INTERP_FILTER_SIZE * 2;
    const bool continuePlayback = fetchCallback(fetchBuffer, samplesRequired);
    const float sincStep = INTERP_FILTER_CUTOFF_FREQ / phaseInc;

    size_t fi = 0;
    for (size_t i = 0; i < buffer.size(); i++) {
        float sampleSum = 0.0f;
        float kernelSum = 0.0f;

        for (int wi = -INTERP_FILTER_SIZE + 1; wi <= INTERP_FILTER_SIZE; wi++) {
            float TiIndexLeft = (float(wi) - phase - 1.0f) * sincStep;
            float TiIndexMiddle = (float(wi) - phase) * sincStep;
            float TiIndexRight = (float(wi) - phase + 1.0f) * sincStep;
            float sl = fast_Ti(TiIndexLeft);
            float sm = fast_Ti(TiIndexMiddle);
            float sr = fast_Ti(TiIndexRight);
            float kernel = sr - 2.0f * sm + sl;
            sampleSum += kernel * fetchBuffer[fi + static_cast<size_t>(wi + INTERP_FILTER_SIZE) - 1];
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
const std::array<float, Resampler::INTERP_FILTER_LUT_SIZE + 2> BlampResampler::TiLut = []() {
    std::array<float, INTERP_FILTER_LUT_SIZE + 2> l;
    double acc = 0.0;
    double step_per_index = double(INTERP_FILTER_SIZE) / double(INTERP_FILTER_LUT_SIZE);
    double integration_inc = step_per_index / double(INTEGRAL_RESOLUTION);
    double index = 0.0;
    double prev_value = 1.0;

    for (size_t i = 0; i < INTERP_FILTER_LUT_SIZE + 1; i++) {
        const double t = double(i) * step_per_index;
        const double convergence_value = t * 0.5;
        const double function_value = t * acc + cos(M_PI * t) / (M_PI * M_PI);
        const double interpolation_t = 0.5 - 0.5 * cos(double(i) * M_PI / double(INTERP_FILTER_LUT_SIZE));
        const double interpolated_value = function_value + interpolation_t * (convergence_value - function_value);
        l[i] = static_cast<float>(interpolated_value);

        for (size_t j = 0; j < INTEGRAL_RESOLUTION; j++) {
            index += integration_inc;
            double new_value = boost::math::sinc_pi(M_PI * index);
            acc += (new_value + prev_value) * integration_inc * 0.5;
            prev_value = new_value;
        }
    }
    l[INTERP_FILTER_LUT_SIZE + 1] = static_cast<float>(((INTERP_FILTER_LUT_SIZE + 1) * step_per_index) * 0.5);
    return l;
}();

float BlampResampler::fast_Ti(float t)
{
    t = std::abs(t);
    const float old_t = t;
    t = std::min(t, float(INTERP_FILTER_SIZE));
    t *= float(double(INTERP_FILTER_LUT_SIZE) / double(INTERP_FILTER_SIZE));
    uint32_t left_index = static_cast<uint32_t>(t);
    float fraction = t - static_cast<float>(left_index);
    uint32_t right_index = left_index + 1;
    float retval = TiLut[left_index] + fraction * (TiLut[right_index] - TiLut[left_index]);
    if (old_t > float(INTERP_FILTER_SIZE))
        return old_t * 0.5f;
    else
        return retval;
}
