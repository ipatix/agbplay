#include "Resampler.h"

#include "NotchFilter.hpp"

#include <algorithm>
#include <numbers>
#include <vector>
#include <cmath>
#include <cstdint>
#include <span>

#include <fmt/core.h>

typedef float PRECISION;
const float SAMPLERATE = 48000.0f;
const size_t NUM_SAMPLES = 1024 * 256;
const float TEST_FREQ = 1000.0f;

template<typename T>
void printSig(std::span<T> buffer)
{
    for (size_t i = 0; i < buffer.size(); i++)
        fmt::print("signal[{}] = {}\n", i, buffer[i]);
}

long double toDB(long double x)
{
    return 20.0 * std::log10(x);
}

template<typename T>
void genSine(std::span<T> buffer, T sampleRate, T toneFreq)
{
    for (size_t i = 0; i < buffer.size(); i++) {
        const T omega0 = 2.0f * std::numbers::pi_v<T> * (toneFreq/sampleRate);
        buffer[i] = std::sin(static_cast<T>(i) * omega0);
    }
}

template<typename T>
void applyWindow(std::span<T> buffer)
{
    const T scale = std::numbers::pi_v<T> / static_cast<T>(buffer.size());
    for (size_t i = 0; i < buffer.size(); i++) {
        buffer[i] *= -0.5 * std::cos(static_cast<T>(i) * scale) + 0.5f;
    }
}

template<typename T>
void applyNotch(std::span<T> buffer, T sampleRate, T filterFreq)
{
    const T Q = 10.0f;

    NotchFilter<T, double> nf;
    nf.setParameters(sampleRate, filterFreq, Q);

    for (size_t i = 0; i < buffer.size(); i++) {
        buffer[i] = nf.process(buffer[i]);
    }
}

template<typename T>
long double calcRms(std::span<T> buffer)
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
    std::vector<PRECISION> buffer(NUM_SAMPLES);

    fmt::print("Measuring aliasing of sine\n");

    fmt::print("Generating sine\n");
    genSine<PRECISION>(buffer, SAMPLERATE, TEST_FREQ);
    fmt::print("rms of sine: {:.4f} dB\n", toDB(calcRms<PRECISION>(buffer)));

    //applyWindow<PRECISION>(buffer);
    //fmt::print("rms of window: {:.4f} dB\n", toDB(calcRms<PRECISION>(buffer)));

    applyNotch<PRECISION>(buffer, SAMPLERATE, 1.0f * TEST_FREQ);

    // delete unstable part of notch filter
    buffer.erase(buffer.begin(), buffer.begin() + 10000);

    fmt::print("rms of notch'ed signal: {:.4f} dB\n", toDB(calcRms<PRECISION>(buffer)));
    printSig<PRECISION>(buffer);
}
