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
            SoundMixer(uint32_t sampleRate, uint32_t fixedModeRate, int reverb, float mvl);
            ~SoundMixer();
            void NewSoundChannel(void *owner, SampleInfo sInfo, ADSR env, Note note, uint8_t leftVol, uint8_t rightVol, int16_t pitch, bool fixed);
            void NewCGBNote(void *owner, CGBDef def, ADSR env, Note note, uint8_t leftVol, uint8_t rightVol, int16_t pitch, CGBType type);
            void SetTrackPV(void *owner, uint8_t volLeft, uint8_t volRight, int16_t pitch);
            // optional FIXME: reduce complexity by replacing the owner pointers with int pointers to a note reference counter so the note amount tracking becomes obsolete
            int TickTrackNotes(void *owner, std::bitset<NUM_NOTES>& activeNotes);
            void StopChannel(void *owner, uint8_t key);
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

            ReverbEffect *revdsp;
            std::vector<float> sampleBuffer;
            uint32_t sampleRate;
            uint32_t fixedModeRate;
            uint32_t samplesPerBuffer;
            float sampleRateReciprocal;

            // volume control related stuff

            float masterVolume;
            float fadePos;
            float fadeStepPerMicroframe;
            uint32_t fadeMicroframesLeft;
    };
}
