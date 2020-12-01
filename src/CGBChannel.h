#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>

#include "Types.h"
#include "Resampler.h"

#define INVALID_OWNER 0xFF

#define NOISE_SAMPLING_FREQ 65536.0f

class CGBChannel
{
public: 
    CGBChannel();
    CGBChannel(const CGBChannel&) = delete;
    CGBChannel& operator=(const CGBChannel&) = delete;

    virtual void Init(uint8_t owner, CGBDef def, Note note, ADSR env);
    virtual void Process(float *buffer, size_t nblocks, MixingArgs& args) = 0;
    uint8_t GetOwner();
    void SetVol(uint8_t vol, int8_t pan);
    uint8_t GetMidiKey();
    int8_t GetNoteLength();
    void Release();
    virtual void SetPitch(int16_t pitch) = 0;
    bool TickNote(); // returns true if note remains active
    EnvState GetState();
protected:
    virtual void stepEnvelope();
    void updateVolFade();
    ChnVol getVol();
    enum class Pan { LEFT, CENTER, RIGHT };
    uint32_t pos;
    float freq;
    ADSR env;
    Note note;
    CGBDef def;
    EnvState eState;
    EnvState nextState;
    Pan pan;
    std::unique_ptr<Resampler> rs;
    uint8_t envInterStep;
    uint8_t envLevel;
    uint8_t envPeak;
    uint8_t envSustain;
    // these values are always 1 frame behind in order to provide a smooth transition
    Pan fromPan;
    uint8_t fromEnvLevel;
    uint8_t owner;
};

class SquareChannel : public CGBChannel
{
public:
    SquareChannel();

    void Init(uint8_t owner, CGBDef def, Note note, ADSR env) override;
    void SetPitch(int16_t pitch) override;
    void Process(float *buffer, size_t nblocks, MixingArgs& args) override;

    const float *pat;
private:
    static bool sampleFetchCallback(std::vector<float>& fetchBuffer, size_t samplesRequired, void *cbdata);
};

class WaveChannel : public CGBChannel
{
public:
    WaveChannel();

    void Init(uint8_t owner, CGBDef def, Note note, ADSR env) override;
    void SetPitch(int16_t pitch) override;
    void Process(float *buffer, size_t nblocks, MixingArgs& args) override;
private:
    static bool sampleFetchCallback(std::vector<float>& fetchBuffer, size_t samplesRequired, void *cbdata);
    float waveBuffer[32];
    static uint8_t volLut[16];
};

class NoiseChannel : public CGBChannel
{
public:
    NoiseChannel();

    void Init(uint8_t owner, CGBDef def, Note note, ADSR env) override;
    void SetPitch(int16_t pitch) override;
    void Process(float *buffer, size_t nblocks, MixingArgs& args) override;
private:
    static bool sampleFetchCallback(std::vector<float>& fetchBuffer, size_t samplesRequired, void *cbdata);
    SincResampler srs;
};
