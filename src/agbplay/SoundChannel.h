#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <span>

#include "Types.h"
#include "MP2KChn.h"

struct MP2KContext;

class SoundChannel : public MP2KChn
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
    SoundChannel(MP2KContext &ctx, MP2KTrack *track, SampleInfo sInfo, ADSR env, const Note& note, bool fixed);
    SoundChannel(const SoundChannel&) = delete;
    SoundChannel& operator=(const SoundChannel&) = delete;

    void Process(std::span<sample> buffer, const MixingArgs& args);
    void SetVol(uint16_t vol, int16_t pan);
    void Release() noexcept override;
    bool IsReleasing() const noexcept;
    void SetPitch(int16_t pitch);
    bool TickNote() noexcept override;
    VoiceFlags GetVoiceType() const noexcept override;
private:
    void stepEnvelope();
    void updateVolFade();
    VolumeFade getVol() const;
    void processNormal(std::span<sample> buffer, ProcArgs& cargs);
    void processModPulse(std::span<sample> buffer, ProcArgs& cargs, float samplesPerBufferInv);
    void processSaw(std::span<sample> buffer, ProcArgs& cargs);
    void processTri(std::span<sample> buffer, ProcArgs& cargs);
    bool sampleFetchCallback(std::vector<float>& fetchBuffer, size_t samplesRequired);
    bool sampleFetchCallbackMPTDecomp(std::vector<float>& fetchBuffer, size_t samplesRequires);

    MP2KContext &ctx;
    SampleInfo sInfo;
    bool fixed;
    bool isGS;              // is Golden Sun synth
    bool isMPTcompressed;   // is Mario Power Tennis compressed
    int16_t levelMPTcompressed;
    uint8_t shiftMPTcompressed;

    /* all of these values have pairs of new and old value to allow smooth fades */
    uint8_t envInterStep = 0;
    uint8_t envLevelCur;
    uint8_t envLevelPrev;
    uint8_t leftVolCur = 0;
    uint8_t leftVolPrev;
    uint8_t rightVolCur = 0;
    uint8_t rightVolPrev;
};
