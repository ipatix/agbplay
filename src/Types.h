#pragma once

#include <cstdint>
#include <string>

enum class CGBType : int { SQ1 = 0, SQ2, WAVE, NOISE };
enum class EnvState : int { INIT = 0, ATK, DEC, SUS, REL, PSEUDO_ECHO, DIE, DEAD };
enum class WaveDuty : int { D12 = 0, D25, D50, D75 };
enum class NoisePatt : int { FINE = 0, ROUGH };
enum class ReverbType { NORMAL, GS1, GS2, MGAT, TEST, NONE };
enum class ResamplerType { NEAREST, LINEAR, SINC, BLEP, BLAMP };
enum class CGBPolyphony { MONO_STRICT, MONO_SMOOTH, POLY };

ReverbType str2rev(const std::string& str);
std::string rev2str(ReverbType t);
ResamplerType str2res(const std::string& str);
std::string res2str(ResamplerType t);
CGBPolyphony str2cgbPoly(const std::string& str);
std::string cgbPoly2str(CGBPolyphony t);

union CGBDef
{
    const uint8_t *wavePtr;
    WaveDuty wd;
    NoisePatt np;
};

struct MixingArgs
{
    float vol;
    uint32_t fixedModeRate;
    float sampleRateInv;
    float samplesPerBufferInv;
};

struct VolumeFade
{
    float fromVolLeft;
    float fromVolRight;
    float toVolLeft;
    float toVolRight;
};

struct ADSR
{
    ADSR(uint8_t att, uint8_t dec, uint8_t sus, uint8_t rel);
    ADSR();
    uint8_t att;
    uint8_t dec;
    uint8_t sus;
    uint8_t rel;
};

struct Note
{
    uint8_t length;
    uint8_t midiKeyTrackData;
    uint8_t midiKeyPitch;
    uint8_t velocity;
    uint8_t priority;
    int8_t rhythmPan;
    uint8_t pseudoEchoVol;
    uint8_t pseudoEchoLen;
    uint8_t trackIdx;
    uint32_t noteId;
};

struct SampleInfo
{
    SampleInfo(const int8_t *samplePtr, float midCfreq, bool loopEnabled, uint32_t loopPos, uint32_t endPos);
    SampleInfo();
    const int8_t *samplePtr;
    float midCfreq;
    uint32_t loopPos;
    uint32_t endPos;
    bool loopEnabled;
};

struct EnginePars
{
    EnginePars(uint8_t vol, uint8_t rev, uint8_t freq);

    uint8_t vol;
    uint8_t rev;
    uint8_t freq;
};

struct SongInfo
{
    size_t songHeaderPos;
    size_t voiceTablePos;
    uint8_t reverb;
    uint8_t priority;
};

struct sample {
    float left;
    float right;
};
