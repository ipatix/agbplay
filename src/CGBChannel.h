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
    CGBChannel(ADSR env, Note note);
    CGBChannel(const CGBChannel&) = delete;
    CGBChannel& operator=(const CGBChannel&) = delete;
    virtual ~CGBChannel() = default;

    virtual void Process(sample *buffer, size_t numSamples, MixingArgs& args) = 0;
    uint8_t GetTrackIdx() const;
    void SetVol(uint8_t vol, int8_t pan);
    const Note& GetNote() const;
    void Release(bool fastRelease = false);
    virtual void SetPitch(int16_t pitch) = 0;
    bool TickNote(); // returns true if note remains active
    EnvState GetState() const;
    bool IsReleasing() const;
    bool IsFastReleasing() const;
protected:
    virtual void stepEnvelope();
    void updateVolFade();
    void applyVol();
    VolumeFade getVol() const;

    std::unique_ptr<Resampler> rs;
    enum class Pan { LEFT, CENTER, RIGHT };
    uint32_t pos = 0;
    float freq = 0.0f;
    ADSR env;
    Note note;
    bool stop = false;
    bool fastRelease = false;
    uint8_t vol = 0;
    int8_t pan = 0;

    /* all of these values have pairs of new and old value to allow smooth fades */
    EnvState envState = EnvState::INIT;
    uint8_t envInterStep = 0;
    uint8_t envLevelCur = 0;
    uint8_t envLevelPrev = 0;
    uint8_t envPeak = 0;
    uint8_t envSustain = 0;
    uint8_t envFrameCount = 0;
    uint8_t envGradientFrame = 0;
    float envGradient = 0.0f;
    Pan panCur = Pan::CENTER;
    Pan panPrev = Pan::CENTER;
};

class SquareChannel : public CGBChannel
{
public:
    SquareChannel(WaveDuty wd, ADSR env, Note note);

    void SetPitch(int16_t pitch) override;
    void Process(sample *buffer, size_t numSamples, MixingArgs& args) override;

    const float *pat = nullptr;
private:
    static bool sampleFetchCallback(std::vector<float>& fetchBuffer, size_t samplesRequired, void *cbdata);
};

class WaveChannel : public CGBChannel
{
public:
    WaveChannel(const uint8_t *wavePtr, ADSR env, Note note);

    void SetPitch(int16_t pitch) override;
    void Process(sample *buffer, size_t numSamples, MixingArgs& args) override;
private:
    VolumeFade getVol() const;
    static bool sampleFetchCallback(std::vector<float>& fetchBuffer, size_t samplesRequired, void *cbdata);
    float waveBuffer[32];
    static uint8_t volLut[16];
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
    NoisePatt np;
};
