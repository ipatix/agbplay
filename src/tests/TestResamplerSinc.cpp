#include "NotchFilter.hpp"
#include "Resampler.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <print>
#include <numbers>
#include <span>
#include <vector>

typedef float PRECISION;
const float SAMPLERATE = 48000.0f;
const float TEST_LEN_S = 2.0f;
const float TEST_FREQ = 22000.0f;

template<typename T> void printSig(std::span<T> buffer)
{
    for (size_t i = 0; i < buffer.size(); i++)
        std::println("signal[{}] = {}", i, buffer[i]);
}

long double toDB(long double x)
{
    return 20.0 * std::log10(x);
}

template<typename T> void genSine(std::span<T> buffer, T sampleRate, T toneFreq)
{
    for (size_t i = 0; i < buffer.size(); i++) {
        const T omega0 = 2.0f * std::numbers::pi_v<T> * (toneFreq / sampleRate);
        buffer[i] = std::sin(static_cast<T>(i) * omega0);
    }
}

template<typename T> void applyWindow(std::span<T> buffer)
{
    const T scale = std::numbers::pi_v<T> / static_cast<T>(buffer.size());
    for (size_t i = 0; i < buffer.size(); i++) {
        buffer[i] *= -0.5 * std::cos(static_cast<T>(i) * scale) + 0.5f;
    }
}

template<typename T> void applyNotch(std::span<T> buffer, T sampleRate, T filterFreq)
{
    const T Q = 10.0f;

    NotchFilter<T, double> nf;
    nf.setParameters(sampleRate, filterFreq, Q);

    for (size_t i = 0; i < buffer.size(); i++) {
        buffer[i] = nf.process(buffer[i]);
    }
}

template<typename T> long double calcRms(std::span<T> buffer)
{
    long double square = 0.0;
    for (size_t i = 100; i < buffer.size(); i++) {
        square = std::fma(buffer[i], buffer[i], square);
    }
    const long double mean = square / static_cast<long double>(buffer.size());
    const long double root = std::sqrt(mean);
    return root;
}

int main()
{
    const size_t NUM_SAMPLES = static_cast<size_t>(TEST_LEN_S * SAMPLERATE);
    std::vector<PRECISION> buffer(NUM_SAMPLES);

    const float resampledRatio = 6.36391f;
    const float resampledRate = SAMPLERATE * resampledRatio;
    std::vector<PRECISION> resampledBuffer(static_cast<size_t>(resampledRatio * static_cast<float>(buffer.size())));

    size_t pos = 0;

    auto sampleFetchFunc = [&](std::vector<float> &fetchBuffer, size_t samplesRequired) {
        if (fetchBuffer.size() >= samplesRequired)
            return true;

        std::println("hello: samplesRequired={}", samplesRequired);
        size_t samplesToFetch = samplesRequired - fetchBuffer.size();
        size_t i = fetchBuffer.size();
        fetchBuffer.resize(samplesRequired);

        do {
            size_t samplesTilEnd = buffer.size() - pos;
            size_t thisFetch = std::min(samplesTilEnd, samplesToFetch);

            samplesToFetch -= thisFetch;
            do {
                fetchBuffer[i++] = buffer[pos++];
            } while (--thisFetch > 0);

            if (pos >= buffer.size()) {
                std::println("warning padding resampler with zeroes: {}", fetchBuffer.size() - i);
                std::fill(fetchBuffer.begin() + i, fetchBuffer.end(), 0.0f);
                return false;
            }
        } while (samplesToFetch > 0);
        return true;
    };

    std::println("Generating sine");
    genSine<PRECISION>(buffer, SAMPLERATE, TEST_FREQ);
    std::println("rms of sine: {:.4f} dB", toDB(calcRms<PRECISION>(buffer)));

    std::println("Resampling");
    SincResampler res;
    res.Process(resampledBuffer, 1.0f / resampledRatio, sampleFetchFunc);
    std::println("rms of resampled sine: {:.4f} dB", toDB(calcRms<PRECISION>(resampledBuffer)));

    // applyWindow<PRECISION>(buffer);
    // std::println("rms of window: {:.4f} dB", toDB(calcRms<PRECISION>(buffer)));

    applyNotch<PRECISION>(buffer, SAMPLERATE, TEST_FREQ);
    applyNotch<PRECISION>(resampledBuffer, resampledRate, TEST_FREQ);

    std::println("Trimming start glitches");
    buffer.erase(buffer.begin(), buffer.begin() + static_cast<int>(0.25f * SAMPLERATE));
    resampledBuffer.erase(resampledBuffer.begin(), resampledBuffer.begin() + static_cast<int>(0.25f * resampledRate));

    std::println("rms of notch'ed signal: {:.4f} dB", toDB(calcRms<PRECISION>(buffer)));
    std::println("rms of notch'ed resampled signal: {:.4f} dB", toDB(calcRms<PRECISION>(resampledBuffer)));
}
