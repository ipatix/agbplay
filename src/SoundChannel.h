#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>

#include "Types.h"
#include "Resampler.h"

class SoundChannel
{
private:
    struct ProcArgs
    {
        float lVol;
        float rVol;
        float lVolStep;
        float rVolStep;
        float interStep;
    };
public:
    SoundChannel(SampleInfo sInfo, ADSR env, const Note& note, bool fixed);
    SoundChannel(const SoundChannel&) = delete;
    SoundChannel& operator=(const SoundChannel&) = delete;

    void Process(sample *buffer, size_t numSamples, const MixingArgs& args);
    uint8_t GetTrackIdx() const;
    void SetVol(uint8_t vol, int8_t pan);
    const Note& GetNote() const;
    void Release();
    void Kill();
    void SetPitch(int16_t pitch);
    bool TickNote(); // returns true if note remains active
    EnvState GetState() const;
private:
    void stepEnvelope();
    void updateVolFade();
    VolumeFade getVol() const;
    void processNormal(sample *buffer, size_t numSamples, ProcArgs& cargs);
    void processModPulse(sample *buffer, size_t numSamples, ProcArgs& cargs, float nBlocksReciprocal);
    void processSaw(sample *buffer, size_t numSamples, ProcArgs& cargs);
    void processTri(sample *buffer, size_t numSamples, ProcArgs& cargs);
    static bool sampleFetchCallback(std::vector<float>& fetchBuffer, size_t samplesRequired, void *cbdata);
    static bool sampleFetchCallbackMPTDecomp(std::vector<float>& fetchBuffer, size_t samplesRequires, void *cbdata);

    std::unique_ptr<Resampler> rs;
    uint32_t pos = 0;
    float interPos = 0.0f;
    float freq = 0.0f;
    ADSR env;
    Note note;
    SampleInfo sInfo;
    bool stop = false;
    bool fixed;
    bool isGS;              // is Golden Sun synth
    bool isMPTcompressed;   // is Mario Power Tennis compressed
    int16_t levelMPTcompressed;
    uint8_t shiftMPTcompressed;

    /* all of these values have pairs of new and old value to allow smooth fades */
    EnvState envState = EnvState::INIT;
    uint8_t envInterStep = 0;
    uint8_t envLevelCur;
    uint8_t envLevelPrev;
    uint8_t leftVolCur = 0;
    uint8_t leftVolPrev;
    uint8_t rightVolCur = 0;
    uint8_t rightVolPrev;
};
