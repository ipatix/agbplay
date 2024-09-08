#pragma once

#include "Resampler.hpp"

#if __has_include(<immintrin.h>)

#include <immintrin.h>

class SincResamplerAVX2 : public SincResampler
{
public:
    ~SincResamplerAVX2() override;
    bool Process(std::span<float> buffer, float phaseInc, const FetchCallback &fetchCallback) override;

private:
    static __m256 fast_sinf(__m256 t);
    static __m256 fast_cosf(__m256 t);
    static __m256 fast_sincf(__m256 t);
    static __m256 window_func(__m256 t);
};

class BlepResamplerAVX2 : public BlepResampler
{
public:
    ~BlepResamplerAVX2() override;
    virtual bool Process(std::span<float> buffer, float phaseInc, const FetchCallback &fetchCallback) override;

private:
    static __m256 fast_Si(__m256 t);
};

class BlampResamplerAVX2 : public BlampResampler
{
public:
    ~BlampResamplerAVX2() override;
    bool Process(std::span<float> buffer, float phaseInc, const FetchCallback &fetchCallback) override;

private:
    static __m256 fast_Ti(__m256 t);
};

#else    // if AVX2 intrinsics header not available

#include <stdexcept>

class SincResamplerAVX2 : public SincResampler
{
public:
    SincResamplerAVX2()
    {
        throw std::logic_error("Attempting to instantiate SincResamplerAVX2 on platform without AVX2");
    }
};

class BlepResamplerAVX2 : public BlepResampler
{
public:
    BlepResamplerAVX2()
    {
        throw std::logic_error("Attempting to instantiate BlepResamplerAVX2 on platform without AVX2");
    }
};

class BlampResamplerAVX2 : public BlampResampler
{
public:
    BlampResamplerAVX2()
    {
        throw std::logic_error("Attempting to instantiate BlampResamplerAVX2 on platform without AVX2");
    }
};

#endif
