#pragma once

#include "SoundData.h"

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

            uint32_t GetBufferElementCount();
            void *ProcessAudio();
        private:
            Sequence& seq;
            EnginePars ep;
            SoundMixer sm;
    };
}
