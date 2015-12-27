#pragma once

#include <vector>
#include <list>
#include <cstdint>

#include "SampleStructs.h"

#define NOTE_TIE -1
// AGB has 60 FPS based processing
#define AGB_FPS 60
// for increased quality we process in subframes (including the base frame)
#define INTERFRAMES 4
// stereo, so 2 channels
#define N_CHANNELS 2
// enable pan snap for CGB
#define CGB_PAN_SNAP

namespace agbplay
{
    class SoundChannel
    {
        public:
            SoundChannel(void *owner, SampleInfo sInfo, ADSR env, Note note, uint8_t leftVol, uint8_t rightVol, int16_t pitch, bool fixed);
            ~SoundChannel();
            void *GetOwner();
            float GetFreq();
            void SetVol(uint8_t leftVol, uint8_t rightVol);
            uint8_t GetVolL();
            uint8_t GetVolR();
            uint8_t GetMidiKey();
            void Release();
            void SetPitch(int16_t pitch);
            bool TickNote(); // returns true if note remains active
            EnvState GetState();
            int8_t *samplePtr;
            float interPos;
        private:
            void *owner;
            float freq;
            bool fixed;
            ADSR env;
            Note note;
            SampleInfo sInfo;
            EnvState eState;
            uint8_t leftVol;
            uint8_t rightVol;
            uint8_t envLevel;

            uint8_t processLeftVol;
            uint8_t processRightVol;
    };

    class CGBChannel
    {
        public: 
            CGBChannel(CGBType t);
            ~CGBChannel();
            void Init(void *owner, CGBDef def, Note note, ADSR env);
            void *GetOwner();
            float GetFreq();
            void SetVol(uint8_t leftVol, uint8_t rightVol);
            uint8_t GetVolL();
            uint8_t GetVolR();
            uint8_t GetMidiKey();
            void Release();
            void SetPitch(int16_t pitch);
            bool TickNote(); // returns true if note remains active
            EnvState GetState();
            float interPos;
        private:
            void *owner;
            float freq;
            ADSR env;
            Note note;
            CGBDef def;
            CGBType cType;
            EnvState eState;
            uint8_t leftVol;
            uint8_t rightVol;
            uint8_t envLevel;

            uint8_t processLeftVol;
            uint8_t processRightVol;
    };

    class SoundMixer
    {
        public:
            SoundMixer(uint32_t sampleRate, uint32_t fixedModeRate);
            ~SoundMixer();
            void NewSoundChannel(void *owner, SampleInfo sInfo, ADSR env, Note note, uint8_t leftVol, uint8_t rightVol, int16_t pitch, bool fixed);
            void NewCGBNote(void *owner, CGBDef def, ADSR env, Note note, uint8_t leftVol, uint8_t rightVol, int16_t pitch, CGBType type);
            void SetTrackPV(void *owner, uint8_t volLeft, uint8_t volRight, int16_t pitch);
            // optional FIXME: reduce complxity by replacing the owner pointers with int pointers to a note reference counter so the note amount tracking becomes obsolete
            int TickTrackNotes(void *owner);
            void StopChannel(void *owner, uint8_t key);
            void *ProcessAndGetAudio();
            uint32_t GetBufferUnitCount();

        private:
            void purgeChannels();

            // channel management
            std::list<SoundChannel> sndChannels;
            CGBChannel sq1;
            CGBChannel sq2;
            CGBChannel wave;
            CGBChannel noise;

            std::vector<float> sampleBuffer;
            uint32_t sampleRate;
            uint32_t fixedModeRate;
            uint32_t samplesPerBuffer;
    };
}
