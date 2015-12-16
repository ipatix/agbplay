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
    enum class CGBType : int { SQ1, SQ2, WAVE, NOISE };
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

    struct SampleInfo
    {
        SampleInfo(int8_t *samplePtr, float midCfreq, bool loopEnabled, uint32_t loopPos, uint32_t endPos);
        SampleInfo();
        int8_t *samplePtr;
        float midCfreq;
        uint32_t loopPos;
        uint32_t endPos;
        bool loopEnabled;
    };

    class SoundChannel
    {
        public:
            SoundChannel(void *owner, SampleInfo sInfo, ADSR env, Note note, uint8_t leftVol, uint8_t rightVol, int16_t pitch);
            ~SoundChannel();
            void *GetOwner();
            float GetFreq();
            void SetVol(uint8_t leftVol, uint8_t rightVol);
            uint8_t GetVolL();
            uint8_t GetVolR();
            void SetPitch(int16_t pitch);
            int8_t *samplePtr;
            float interPos;
        private:
            void *owner;
            float freq;
            ADSR env;
            Note note;
            SampleInfo sInfo;
            uint8_t leftVol;
            uint8_t rightVol;

            // do not touch these values, the mixing engine cares about these
            // they are used for the volume change micro ramping
            // the mixer will fade the values of process* over to the according values
            uint8_t processLeftVol;
            uint8_t processRightVol;
            uint8_t processEnvelope;
    };

    class CGBChannel
    {
        public: 
            CGBChannel();
            ~CGBChannel();
            void Init(void *owner, Note note, ADSR env);
            void *GetOwner();
            float GetFreq();
            void SetVol(uint8_t leftVol, uint8_t rightVol);
            uint8_t GetVolL();
            uint8_t GetVolR();
            void SetPitch(int16_t pitch);
            uint8_t *wavePtr;
            float interPos;
        private:
            void *owner;
            float freq;
            ADSR env;
            Note note;
            uint8_t leftVol;
            uint8_t rightVol;

            uint8_t processLeftVol;
            uint8_t processRightVol;
            uint8_t processEnvelope;
    };

    class SoundMixer
    {
        public:
            SoundMixer(uint32_t sampleRate);
            ~SoundMixer();
            void NewSoundChannel(void *owner, SampleInfo sInfo, ADSR env, Note note, uint8_t leftVol, uint8_t rightVol, int16_t pitch);
            void NewCGBChannel(void *owner, ADSR env, Note note, uint8_t leftVol, uint8_t rightVol, int16_t pitch, uint8_t chn);
            void SetAllTrackPars(void *owner, uint8_t volLeft, uint8_t volRight, int16_t pitch);
            void TickAllTrackNotes(void *owner);
            void DelChannel(void *owner, uint8_t key);
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
