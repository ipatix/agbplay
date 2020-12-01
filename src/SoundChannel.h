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
    SoundChannel(uint8_t owner, SampleInfo sInfo, ADSR env, Note note, uint8_t vol, int8_t pan, int16_t pitch, bool fixed);
    SoundChannel(const SoundChannel&) = delete;
    SoundChannel& operator=(const SoundChannel&) = delete;

    void Process(float *buffer, size_t nblocks, const MixingArgs& args);
    uint8_t GetOwner();
    void SetVol(uint8_t vol, int8_t pan);
    uint8_t GetMidiKey();
    int8_t GetNoteLength();
    void Release();
    void Kill();
    void SetPitch(int16_t pitch);
    bool TickNote(); // returns true if note remains active
    EnvState GetState();
    SampleInfo& GetInfo();
    uint8_t GetInterStep();
private:
    void stepEnvelope();
    void updateVolFade();
    ChnVol getVol();
    void processNormal(float *buffer, size_t nblocks, ProcArgs& cargs);
    void processModPulse(float *buffer, size_t nblocks, ProcArgs& cargs, float nBlocksReciprocal);
    void processSaw(float *buffer, size_t nblocks, ProcArgs& cargs);
    void processTri(float *buffer, size_t nblocks, ProcArgs& cargs);
    static bool sampleFetchCallback(std::vector<float>& fetchBuffer, size_t samplesRequired, void *cbdata);
    static bool sampleFetchCallbackMPTDecomp(std::vector<float>& fetchBuffer, size_t samplesRequires, void *cbdata);
    std::unique_ptr<Resampler> rs;
    uint32_t pos;
    float interPos;
    float freq;
    ADSR env;
    Note note;
    SampleInfo sInfo;
    EnvState eState;
    bool fixed;
    bool isGS;              // is Golden Sun synth
    bool isMPTcompressed;   // is Mario Power Tennis compressed
    int16_t levelMPTcompressed;
    uint8_t shiftMPTcompressed;
    uint8_t owner;
    uint8_t envInterStep;
    uint8_t leftVol;
    uint8_t rightVol;
    uint8_t envLevel;
    // these values are always 1 frame behind in order to provide a smooth transition
    uint8_t fromLeftVol;
    uint8_t fromRightVol;
    uint8_t fromEnvLevel;
};
