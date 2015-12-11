#pragma once

#include <vector>
#include <list>
#include <cstdint>

#define NOTE_TIE -1
// AGB has 60 FPS based processing
#define AGB_FPS 60
// for increased quality we process in subframes (including the base frame)
#define INTERFRAMES 4
// stereo, so 2 channels
#define N_CHANNELS 2

namespace agbplay
{
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
        Note(uint8_t midiKey, uint8_t velocity, int8_t length);
        Note();
        uint8_t midiKey;
        uint8_t velocity;
        int8_t length;
    };

    class SoundChannel
    {
        public:
            SoundChannel(int8_t *samplePtr, ADSR env, Note note);
            ~SoundChannel();
            void SetFreq(float freq);

            int8_t *samplePtr;
            float freq;
            float interPos;

            SoundChannel *trNext;
            SoundChannel *trPrev;
        private:
            uint32_t sampLength;
            bool loopEnabled;
            ADSR env;
            Note note;
    };

    class CGBChannel
    {
        public: 
            CGBChannel();
            ~CGBChannel();
    };

    class SoundMixer
    {
        public:
            SoundMixer(uint32_t sampleRate);
            ~SoundMixer();
            SoundChannel *AllocateChannel();
            void *ProcessAndGetAudio();
            uint32_t GetBufferUnitCount();

        private:
            // channel management
            std::list<SoundChannel> sndChannels;
            CGBChannel sq1;
            CGBChannel sq2;
            CGBChannel wave;
            CGBChannel noise;

            std::vector<float> sampleBuffer;
            uint32_t sampleRate;
            uint32_t samplesPerBuffer;
    };
}
