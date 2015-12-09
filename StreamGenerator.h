#pragma once

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
            StreamGenerator(Sequence& seq, uint32_t outSampleRate, EnginePars ep);
            ~StreamGenerator();

            uint32_t GetBufferUnitCount();
            void *ProcessAndGetAudio();
        private:
            Sequence& seq;
            EnginePars ep;
            SoundMixer sm;

            void processSequenceFrame();
            void processSequenceTick();
    };
}
