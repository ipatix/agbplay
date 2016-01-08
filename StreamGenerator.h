#pragma once

#include <map>

#include "SoundData.h"
#include "SoundMixer.h"

#define BPM_PER_FRAME 150

namespace agbplay
{
    struct EnginePars
    {
        EnginePars(uint8_t vol, uint8_t rev, uint8_t freq);
        EnginePars();

        uint8_t vol;
        uint8_t rev;
        uint8_t freq;
    };

    class StreamGenerator
    {
        public:
            StreamGenerator(Sequence& seq, EnginePars ep, uint8_t maxLoops);
            ~StreamGenerator();

            uint32_t GetBufferUnitCount();
            uint32_t GetRenderSampleRate();
            float *ProcessAndGetAudio();
            static const std::map<uint8_t, int8_t> delayLut;
            static const std::map<uint8_t, int8_t> noteLut;

        private:
            Sequence seq;
            SoundBank sbnk;
            EnginePars ep;
            SoundMixer sm;

            static const std::vector<uint32_t> freqLut;

            bool isEnding;
            uint8_t maxLoops;

            void processSequenceFrame();
            void processSequenceTick();
            void playNote(Sequence::Track& trk, Note note, void *owner);
    };
}
