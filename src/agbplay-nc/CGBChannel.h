#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>

#include "Types.h"
#include "Resampler.h"

#define INVALID_TRACK_IDX 0xFF

#define NOISE_SAMPLING_FREQ 65536.0f

class CGBChannel
{
public: 
    CGBChannel(ADSR env, Note note, bool useStairstep = false);
    CGBChannel(const CGBChannel&) = delete;
    CGBChannel& operator=(const CGBChannel&) = delete;
    virtual ~CGBChannel() = default;

    virtual void Process(sample *buffer, size_t numSamples, MixingArgs& args) = 0;
    uint8_t GetTrackIdx() const;
    void SetVol(uint16_t vol, int16_t pan);
    const Note& GetNote() const;
    void Release(bool fastRelease = false);
    virtual void SetPitch(int16_t pitch) = 0;
    bool TickNote(); // returns true if note remains active
    EnvState GetState() const;
    bool IsReleasing() const;
    bool IsFastReleasing() const;
protected:
    virtual bool IsChn3() const;
    void stepEnvelope();
    void updateVolFade();
    void applyVol();
    VolumeFade getVol() const;
    uint8_t getPseudoEchoLevel() const;

    static float timer2freq(float timer);
    static float freq2timer(float freq);

    std::unique_ptr<Resampler> rs;
    enum class Pan { LEFT, CENTER, RIGHT };
    uint32_t pos = 0;
    float freq = 0.0f;
    ADSR env;
    Note note;
    const bool useStairstep;
    bool stop = false;
    bool fastRelease = false;
    uint16_t vol = 0;
    int16_t pan = 0;
    bool mp2k_sus_vol_bug_update = false;

    /* all of these values have pairs of new and old value to allow smooth fades */
    EnvState envState = EnvState::INIT;
    uint8_t envInterStep = 0;
    uint8_t envLevelCur = 0;
    uint8_t envPeak = 0;
    uint8_t envSustain = 0;
    uint8_t envFrameCount = 0;
    float envFadeLevel = 0.0f;
    VolumeFade volFade{0.0f, 0.0f, 0.0f, 0.0f};
    Pan panCur = Pan::CENTER;
    Pan panPrev = Pan::CENTER;
};

class SquareChannel : public CGBChannel
{
public:
    SquareChannel(WaveDuty wd, ADSR env, Note note, uint8_t sweep);

    void SetPitch(int16_t pitch) override;
    void Process(sample *buffer, size_t numSamples, MixingArgs& args) override;
private:
    static bool sampleFetchCallback(std::vector<float>& fetchBuffer, size_t samplesRequired, void *cbdata);

    static bool isSweepEnabled(uint8_t sweep);
    static bool isSweepAscending(uint8_t sweep);
    static float sweep2coeff(uint8_t sweep);
    static float sweep2convergence(uint8_t sweep);
    static uint8_t sweepTime(uint8_t sweep);

    const float *pat = nullptr;
    int16_t sweepStartCount = -1;
    const uint8_t sweep;
    const bool sweepEnabled;
    const float sweepConvergence;
    const float sweepCoeff;
    /* sweepTimer is emulated with float instead of hardware int to get sub frame accuracy */
    float sweepTimer = 1.0f;
};

class WaveChannel : public CGBChannel
{
public:
    WaveChannel(const uint8_t *wavePtr, ADSR env, Note note, bool useStairstep);

    void SetPitch(int16_t pitch) override;
    void Process(sample *buffer, size_t numSamples, MixingArgs& args) override;
private:
    bool IsChn3() const override;
    VolumeFade getVol() const;
    static bool sampleFetchCallback(std::vector<float>& fetchBuffer, size_t samplesRequired, void *cbdata);
    float dcCorrection100;
    float dcCorrection75;
    float dcCorrection50;
    float dcCorrection25;
    const uint8_t * const wavePtr;
};

class NoiseChannel : public CGBChannel
{
public:
    NoiseChannel(NoisePatt np, ADSR env, Note note);

    void SetPitch(int16_t pitch) override;
    void Process(sample *buffer, size_t numSamples, MixingArgs& args) override;
private:
    static bool sampleFetchCallback(std::vector<float>& fetchBuffer, size_t samplesRequired, void *cbdata);
    SincResampler srs;
    uint16_t noiseState;
    uint16_t noiseLfsrMask;
};
