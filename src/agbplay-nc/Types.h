#pragma once

#include <bitset>
#include <cstdint>
#include <string>
#include <vector>

#include "Constants.h"

enum class CGBType : int { SQ1 = 0, SQ2, WAVE, NOISE };
enum class EnvState : int { INIT = 0, ATK, DEC, SUS, REL, PSEUDO_ECHO, DIE, DEAD };
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

struct MixingArgs
{
    float vol;
    uint32_t fixedModeRate;
    float sampleRateInv;
    float samplesPerBufferInv;
    size_t curInterFrame;   // <-- for debugging only
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
    uint8_t att = 0xFF;
    uint8_t dec = 0x00;
    uint8_t sus = 0xFF;
    uint8_t rel = 0x00;
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
    uint8_t playerIdx;
};

struct SampleInfo
{
    const int8_t *samplePtr;
    float midCfreq;
    uint32_t loopPos;
    uint32_t endPos;
    bool loopEnabled;
};

struct MP2KSoundMode
{
    const uint8_t vol;
    const uint8_t rev;
    const uint8_t freq;
    const uint8_t maxChannels = 0; // currently unused
    const uint8_t dacConfig = 0; // currently unused
};

struct AgbplaySoundMode
{
    const ResamplerType resamplerTypeNormal;
    const ResamplerType resamplerTypeFixed;
    const ReverbType reverbType;
    const CGBPolyphony cgbPolyphony;
    const uint32_t dmaBufferLen;
    const int8_t maxLoops;
    const double padSilenceSecondsStart;
    const double padSilenceSecondsEnd;
    const bool accurateCh3Quantization;
    const bool accurateCh3Volume;
    const bool emulateCgbSustainBug; // other places may call this 'simulate', should probably use 'emulate' everywhere
};

struct SongTableInfo
{
    const size_t songTablePos;
    const uint16_t songCount;
};

typedef std::vector<uint8_t> PlayerTableInfo;

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

struct PlaylistEntry {
    std::string name;
    uint16_t id;
};

struct GameMatch {
    std::vector<std::string> gameCodes;
    std::vector<uint8_t> magicBytes;
};

struct MP2KVisualizerState 
{
    MP2KVisualizerState() = default;
    MP2KVisualizerState(const MP2KVisualizerState&) = delete;
    //MP2KVisualizerState& operator=(const MP2KVisualizerState&) = delete;

    struct TrackState
    {
        uint32_t trackPtr = 0;
        bool isCalling = false;
        bool isMuted = false;
        uint8_t vol = 100;              // range 0 to 127
        uint8_t mod = 0;                // range 0 to 127
        uint8_t prog = PROG_UNDEFINED;  // range 0 to 127
        int8_t pan = 0;                 // range -64 to 63
        int16_t pitch = 0;              // range -32768 to 32767
        uint8_t envL = 0;               // range 0 to 255
        uint8_t envR = 0;               // range 0 to 255
        uint8_t delay = 0;              // range 0 to 96
                                        //
        std::bitset<NUM_NOTES> activeNotes;
        float volLeft = 0.0f, volRight = 0.0f;
    };

    struct PlayerState
    {
        std::vector<TrackState> tracks;
        uint16_t bpm = 0;
        uint8_t tracksUsed = 0;
    };

    std::vector<PlayerState> players;

    float masterVolLeft = 0.0f, masterVolRight = 0.0f;
    size_t activeChannels = 0;
    uint8_t primaryPlayer = 0;
};
