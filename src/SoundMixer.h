#pragma once

#include <vector>
#include <list>
#include <cstdint>
#include <bitset>

#include "ReverbEffect.h"
#include "SoundChannel.h"
#include "CGBChannel.h"
#include "Constants.h"

#define NOTE_TIE -1
// AGB has 60 FPS based processing
#define AGB_FPS 60
// stereo, so 2 channels
#define N_CHANNELS 2
// enable pan snap for CGB
#define CGB_PAN_SNAP
#define MASTER_VOL 1.0f

namespace agbplay
{
    class SoundMixer
    {
        public:
            SoundMixer(uint32_t sampleRate, uint32_t fixedModeRate, uint8_t reverb, float mvl, ReverbType rtype);
            ~SoundMixer();
            void NewSoundChannel(uint8_t owner, SampleInfo sInfo, ADSR env, Note note, uint8_t vol, int8_t pan, int16_t pitch, bool fixed);
            void NewCGBNote(uint8_t owner, CGBDef def, ADSR env, Note note, uint8_t vol, int8_t pan, int16_t pitch, CGBType type);
            void SetTrackPV(uint8_t owner, uint8_t vol, int8_t pan, int16_t pitch);
            int TickTrackNotes(uint8_t owner, std::bitset<NUM_NOTES>& activeNotes);
            void StopChannel(uint8_t owner, uint8_t key);
            float *ProcessAndGetAudio();
            uint32_t GetBufferUnitCount();
            uint32_t GetRenderSampleRate();
            void FadeOut(float millis);
            void FadeIn(float millis);
            bool IsFadeDone();

        private:
            void purgeChannels();
            void clearBuffer();
            void renderToBuffer();

            std::bitset<NUM_NOTES> activeBackBuffer;

            // channel management
            std::list<SoundChannel> sndChannels;
            SquareChannel sq1;
            SquareChannel sq2;
            WaveChannel wave;
            NoiseChannel noise;

            ReverbEffect revdsp;
            std::vector<float> sampleBuffer;
            uint32_t sampleRate;
            uint32_t fixedModeRate;
            uint32_t samplesPerBuffer;
            float sampleRateReciprocal;

            // volume control related stuff

            float masterVolume;
            float pcmMasterVolume;
            float fadePos;
            float fadeStepPerMicroframe;
            uint32_t fadeMicroframesLeft;
    };
}
