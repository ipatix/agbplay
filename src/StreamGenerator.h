#pragma once

#include <map>
#include <vector>

#include "Constants.h"
#include "SoundData.h"
#include "SoundMixer.h"

struct EnginePars
{
    EnginePars(uint8_t vol, uint8_t rev, uint8_t freq);

    uint8_t vol;
    uint8_t rev;
    uint8_t freq;
};

class StreamGenerator
{
public:
    StreamGenerator(Sequence& seq, EnginePars ep, uint8_t maxLoops, float speedFactor, ReverbType rtype);
    StreamGenerator(const StreamGenerator&) = delete;
    StreamGenerator& operator=(const StreamGenerator&) = delete;
    ~StreamGenerator();

    size_t GetBufferUnitCount();
    size_t GetActiveChannelCount();
    uint32_t GetRenderSampleRate();
    std::vector<std::vector<float>>& ProcessAndGetAudio();
    bool HasStreamEnded();
    Sequence& GetWorkingSequence();
    void SetSpeedFactor(float speedFactor);

    static const std::map<uint8_t, int8_t> delayLut;
    static const std::map<uint8_t, int8_t> noteLut;

private:
    Sequence& seq;
    SoundBank sbnk;
    EnginePars ep;
    SoundMixer sm;

    static const std::vector<uint32_t> freqLut;

    bool isEnding;
    uint8_t maxLoops;
    float speedFactor;

    void processSequenceFrame();
    void processSequenceTick();
    void playNote(Sequence::Track& trk, Note note, uint8_t owner);
};
