#include "ResamplerAVX2.hpp"

#include <cmath>

/* A few AVX2 helper functions */

static inline __m256 avx2_abs(__m256 x)
{
    const __m256 signMask = _mm256_set1_ps(-0.0f);
    return _mm256_andnot_ps(signMask, x);
}

static inline __m256 avx2_copysign(__m256 x, __m256 s)
{
    const __m256 signMask = _mm256_set1_ps(-0.0f);
    return _mm256_or_ps(_mm256_andnot_ps(signMask, x), _mm256_and_ps(signMask, s));
}

static inline void avx2_hsum2(__m256 va, __m256 vb, float &a, float &b)
{
    // REDUCE va and vb
    /* tmp[a[76],a[54],b[76],b[54] , a[32],a[10],b[32],b[10]] */
    const __m256 tmp = _mm256_hadd_ps(vb, va);
    /* tmplo[a[32],a[10] , b[32],b[10]] */
    const __m128 tmplo = _mm256_castps256_ps128(tmp);
    /* tmphi[a[76],a[54] , b[76],b[54]] */
    const __m128 tmphi = _mm256_extractf128_ps(tmp, 1);
    /* tmp2[a[7632],a[4310] , b[7632],b[5410] */
    const __m128 tmp2 = _mm_add_ps(tmphi, tmplo);
    /* tmp3[a[4310],a[7632] , b[5410],b[7632] */
    const __m128 tmp3 = _mm_shuffle_ps(tmp2, tmp2, 0b10110001);
    /* tmp4[a[76543210],a[76543210] , b[76543210],b[76543210]] */
    const __m128 tmp4 = _mm_add_ps(tmp2, tmp3);

    b = _mm_cvtss_f32(tmp4);
    a = _mm_cvtss_f32(_mm_shuffle_ps(tmp4, tmp4, 0b00000010));
}

SincResamplerAVX2::~SincResamplerAVX2()
{
}

bool SincResamplerAVX2::Process(std::span<float> buffer, float phaseInc, const FetchCallback &fetchCallback)
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
    const __m256 sincStepV = _mm256_set1_ps(sincStep);
    const __m256i sincWinSizeV = _mm256_set1_epi32(INTERP_FILTER_SIZE);

    int32_t fi = 0;
    for (size_t i = 0; i < buffer.size(); i++) {
        __m256 sampleSumV = _mm256_set1_ps(0.0f);
        __m256 kernelSumV = _mm256_set1_ps(0.0f);
        const __m256 phaseV = _mm256_set1_ps(phase);
        __m256i wiV = _mm256_sub_epi32(_mm256_set_epi32(8, 7, 6, 5, 4, 3, 2, 1), sincWinSizeV);

        for (int wi = -INTERP_FILTER_SIZE + 1; wi <= INTERP_FILTER_SIZE;
             wi += 8, wiV = _mm256_add_epi32(wiV, _mm256_set1_epi32(8))) {
            const __m256 sincIndexV = _mm256_mul_ps(_mm256_sub_ps(_mm256_cvtepi32_ps(wiV), phaseV), sincStepV);
            const __m256 windowIndexV = _mm256_sub_ps(_mm256_cvtepi32_ps(wiV), phaseV);

            const __m256 sV = fast_sincf(sincIndexV);
            const __m256 wV = window_func(windowIndexV);
            const __m256 kernelV = _mm256_mul_ps(sV, wV);
            const __m256 fetchedSampleV =
                _mm256_loadu_ps(&fetchBuffer[static_cast<size_t>(fi + wi + INTERP_FILTER_SIZE) - 1]);
            sampleSumV = _mm256_add_ps(sampleSumV, _mm256_mul_ps(kernelV, fetchedSampleV));
            kernelSumV = _mm256_add_ps(kernelSumV, kernelV);
        }

        float kernelSum, sampleSum;
        avx2_hsum2(kernelSumV, sampleSumV, kernelSum, sampleSum);

        phase += phaseInc;
        const int32_t istep = static_cast<int32_t>(phase);
        phase -= static_cast<float>(istep);
        fi += istep;

        buffer[i] = sampleSum / kernelSum;
    }

    fetchBuffer.erase(fetchBuffer.begin(), fetchBuffer.begin() + fi);
    return continuePlayback;
}

inline __m256 SincResamplerAVX2::fast_sinf(__m256 t)
{
    return SincResamplerAVX2::fast_cosf(_mm256_sub_ps(t, _mm256_set1_ps(float(M_PI / 2.0))));
}

inline __m256 SincResamplerAVX2::fast_cosf(__m256 t)
{
    t = avx2_abs(t);
    t = _mm256_mul_ps(t, _mm256_set1_ps(float(double(INTERP_FILTER_LUT_SIZE) / (2.0 * M_PI))));
    __m256i leftIndex = _mm256_cvttps_epi32(t);
    const __m256 fraction = _mm256_sub_ps(t, _mm256_cvtepi32_ps(leftIndex));
    const __m256i rightIndex = _mm256_and_si256(
        _mm256_add_epi32(leftIndex, _mm256_set1_epi32(1)), _mm256_set1_epi32(INTERP_FILTER_LUT_SIZE - 1)
    );
    leftIndex = _mm256_and_si256(leftIndex, _mm256_set1_epi32(INTERP_FILTER_LUT_SIZE - 1));
    const __m256 leftFetch = _mm256_i32gather_ps(cosLut.data(), leftIndex, sizeof(decltype(cosLut)::value_type));
    const __m256 rightFetch = _mm256_i32gather_ps(cosLut.data(), rightIndex, sizeof(decltype(cosLut)::value_type));
    return _mm256_add_ps(leftFetch, _mm256_mul_ps(fraction, _mm256_sub_ps(rightFetch, leftFetch)));
}

inline __m256 SincResamplerAVX2::fast_sincf(__m256 t)
{
    t = avx2_abs(t);
    t = _mm256_mul_ps(t, _mm256_set1_ps(float(double(INTERP_FILTER_LUT_SIZE) / double(INTERP_FILTER_SIZE))));
    const __m256i leftIndex = _mm256_cvttps_epi32(t);
    const __m256 fraction = _mm256_sub_ps(t, _mm256_cvtepi32_ps(leftIndex));
    const __m256i rightIndex = _mm256_add_epi32(leftIndex, _mm256_set1_epi32(1));
    const __m256 leftFetch = _mm256_i32gather_ps(sincLut.data(), leftIndex, sizeof(decltype(sincLut)::value_type));
    const __m256 rightFetch = _mm256_i32gather_ps(sincLut.data(), rightIndex, sizeof(decltype(sincLut)::value_type));
    return _mm256_add_ps(leftFetch, _mm256_mul_ps(fraction, _mm256_sub_ps(rightFetch, leftFetch)));
}

inline __m256 SincResamplerAVX2::window_func(__m256 t)
{
    t = avx2_abs(t);
    t = _mm256_mul_ps(t, _mm256_set1_ps(float(double(INTERP_FILTER_LUT_SIZE) / double(INTERP_FILTER_SIZE))));
    const __m256i leftIndex = _mm256_cvttps_epi32(t);
    const __m256 fraction = _mm256_sub_ps(t, _mm256_cvtepi32_ps(leftIndex));
    const __m256i rightIndex = _mm256_add_epi32(leftIndex, _mm256_set1_epi32(1));
    const __m256 leftFetch = _mm256_i32gather_ps(winLut.data(), leftIndex, sizeof(decltype(winLut)::value_type));
    const __m256 rightFetch = _mm256_i32gather_ps(winLut.data(), rightIndex, sizeof(decltype(winLut)::value_type));
    return _mm256_add_ps(leftFetch, _mm256_mul_ps(fraction, _mm256_sub_ps(rightFetch, leftFetch)));
}

BlepResamplerAVX2::~BlepResamplerAVX2()
{
}

bool BlepResamplerAVX2::Process(std::span<float> buffer, float phaseInc, const FetchCallback &fetchCallback)
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
    const __m256 sincStepV = _mm256_set1_ps(sincStep);
    const __m256i sincWinSizeV = _mm256_set1_epi32(INTERP_FILTER_SIZE);

    int32_t fi = 0;
    for (size_t i = 0; i < buffer.size(); i++) {
        __m256 sampleSumV = _mm256_set1_ps(0.0f);
        __m256 kernelSumV = _mm256_set1_ps(0.0f);
        const __m256 phaseV = _mm256_set1_ps(phase);
        __m256i wiV = _mm256_sub_epi32(_mm256_set_epi32(8, 7, 6, 5, 4, 3, 2, 1), sincWinSizeV);

        const __m256 wi0MPhaseV = _mm256_sub_ps(_mm256_cvtepi32_ps(wiV), phaseV);
        const __m256 SiIndexInitialV = _mm256_mul_ps(_mm256_sub_ps(wi0MPhaseV, _mm256_set1_ps(0.5f)), sincStepV);
        __m256 slV = fast_Si(SiIndexInitialV);

        for (int wi = -INTERP_FILTER_SIZE + 1; wi <= INTERP_FILTER_SIZE;
             wi += 8, wiV = _mm256_add_epi32(wiV, _mm256_set1_epi32(8))) {
            const __m256 wiMPhaseV = _mm256_sub_ps(_mm256_cvtepi32_ps(wiV), phaseV);
            const __m256 SiIndexRightV = _mm256_mul_ps(_mm256_add_ps(wiMPhaseV, _mm256_set1_ps(0.5f)), sincStepV);
            const __m256 srV = fast_Si(SiIndexRightV);
            const __m256 kernelV = _mm256_sub_ps(srV, slV);
            const __m256 fetchedSampleV =
                _mm256_loadu_ps(&fetchBuffer[static_cast<size_t>(fi + wi + INTERP_FILTER_SIZE) - 1]);
            sampleSumV = _mm256_add_ps(sampleSumV, _mm256_mul_ps(kernelV, fetchedSampleV));
            kernelSumV = _mm256_add_ps(kernelSumV, kernelV);
            slV = srV;
        }

        float kernelSum, sampleSum;
        avx2_hsum2(kernelSumV, sampleSumV, kernelSum, sampleSum);
        phase += phaseInc;
        const int32_t istep = static_cast<int32_t>(phase);
        phase -= static_cast<float>(istep);
        fi += istep;

        buffer[i] = sampleSum / kernelSum;
    }

    fetchBuffer.erase(fetchBuffer.begin(), fetchBuffer.begin() + fi);
    return continuePlayback;
}

inline __m256 BlepResamplerAVX2::fast_Si(__m256 t)
{
    __m256 signed_t = t;
    t = avx2_abs(t);
    t = _mm256_min_ps(t, _mm256_set1_ps(float(INTERP_FILTER_SIZE)));
    t = _mm256_mul_ps(t, _mm256_set1_ps(float(double(INTERP_FILTER_LUT_SIZE) / double(INTERP_FILTER_SIZE))));
    const __m256i leftIndex = _mm256_cvttps_epi32(t);
    const __m256 fraction = _mm256_sub_ps(t, _mm256_cvtepi32_ps(leftIndex));
    const __m256i rightIndex = _mm256_add_epi32(leftIndex, _mm256_set1_epi32(1));
    const __m256 leftFetch = _mm256_i32gather_ps(SiLut.data(), leftIndex, sizeof(decltype(SiLut)::value_type));
    const __m256 rightFetch = _mm256_i32gather_ps(SiLut.data(), rightIndex, sizeof(decltype(SiLut)::value_type));
    const __m256 retval = _mm256_add_ps(leftFetch, _mm256_mul_ps(fraction, _mm256_sub_ps(rightFetch, leftFetch)));
    return avx2_copysign(retval, signed_t);
}

BlampResamplerAVX2::~BlampResamplerAVX2()
{
}

bool BlampResamplerAVX2::Process(std::span<float> buffer, float phaseInc, const FetchCallback &fetchCallback)
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
    const __m256 sincStepV = _mm256_set1_ps(sincStep);
    const __m256i sincWinSizeV = _mm256_set1_epi32(INTERP_FILTER_SIZE);

    int32_t fi = 0;
    for (size_t i = 0; i < buffer.size(); i++) {
        __m256 sampleSumV = _mm256_set1_ps(0.0f);
        __m256 kernelSumV = _mm256_set1_ps(0.0f);
        const __m256 phaseV = _mm256_set1_ps(phase);
        __m256i wiV = _mm256_sub_epi32(_mm256_set_epi32(8, 7, 6, 5, 4, 3, 2, 1), sincWinSizeV);

        const __m256 wi0MPhaseV = _mm256_sub_ps(_mm256_cvtepi32_ps(wiV), phaseV);
        const __m256 TiIndexInitialLeftV = _mm256_mul_ps(_mm256_sub_ps(wi0MPhaseV, _mm256_set1_ps(1.0f)), sincStepV);
        const __m256 TiIndexInitialMiddleV = _mm256_mul_ps(wi0MPhaseV, sincStepV);
        __m256 slV = fast_Ti(TiIndexInitialLeftV);
        __m256 smV = fast_Ti(TiIndexInitialMiddleV);

        for (int wi = -INTERP_FILTER_SIZE + 1; wi <= INTERP_FILTER_SIZE;
             wi += 8, wiV = _mm256_add_epi32(wiV, _mm256_set1_epi32(8))) {
            const __m256 wiMPhaseV = _mm256_sub_ps(_mm256_cvtepi32_ps(wiV), phaseV);
            const __m256 TiIndexRightV = _mm256_mul_ps(_mm256_add_ps(wiMPhaseV, _mm256_set1_ps(1.0f)), sincStepV);
            const __m256 srV = fast_Ti(TiIndexRightV);
            const __m256 kernelV = _mm256_add_ps(_mm256_sub_ps(_mm256_sub_ps(srV, smV), smV), slV);
            const __m256 fetchedSampleV =
                _mm256_loadu_ps(&fetchBuffer[static_cast<size_t>(fi + wi + INTERP_FILTER_SIZE) - 1]);
            sampleSumV = _mm256_add_ps(sampleSumV, _mm256_mul_ps(kernelV, fetchedSampleV));
            kernelSumV = _mm256_add_ps(kernelSumV, kernelV);
            slV = smV;
            smV = srV;
        }

        float kernelSum, sampleSum;
        avx2_hsum2(kernelSumV, sampleSumV, kernelSum, sampleSum);

        phase += phaseInc;
        const int32_t istep = static_cast<int32_t>(phase);
        phase -= static_cast<float>(istep);
        fi += istep;

        buffer[i] = sampleSum / kernelSum;
    }

    fetchBuffer.erase(fetchBuffer.begin(), fetchBuffer.begin() + fi);
    return continuePlayback;
}

inline __m256 BlampResamplerAVX2::fast_Ti(__m256 t)
{
    t = avx2_abs(t);
    __m256 old_t = t;
    t = _mm256_min_ps(t, _mm256_set1_ps(float(INTERP_FILTER_SIZE)));
    t = _mm256_mul_ps(t, _mm256_set1_ps(float(double(INTERP_FILTER_LUT_SIZE) / double(INTERP_FILTER_SIZE))));
    const __m256i leftIndex = _mm256_cvttps_epi32(t);
    const __m256 fraction = _mm256_sub_ps(t, _mm256_cvtepi32_ps(leftIndex));
    const __m256i rightIndex = _mm256_add_epi32(leftIndex, _mm256_set1_epi32(1));
    const __m256 leftFetch = _mm256_i32gather_ps(TiLut.data(), leftIndex, sizeof(decltype(TiLut)::value_type));
    const __m256 rightFetch = _mm256_i32gather_ps(TiLut.data(), rightIndex, sizeof(decltype(TiLut)::value_type));
    const __m256 retval = _mm256_add_ps(leftFetch, _mm256_mul_ps(fraction, _mm256_sub_ps(rightFetch, leftFetch)));
    const __m256 outOfRangeRetval = _mm256_mul_ps(old_t, _mm256_set1_ps(0.5f));
    const __m256 isOutOfRange = _mm256_cmp_ps(old_t, _mm256_set1_ps(float(INTERP_FILTER_SIZE)), _CMP_GT_OS);
    return _mm256_or_ps(_mm256_andnot_ps(isOutOfRange, retval), _mm256_and_ps(isOutOfRange, outOfRangeRetval));
}
