#pragma once

#include <cstdint>

#include "SampleStructs.h"

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
            void StepEnvelope();
            int8_t *samplePtr;
            float interPos;
        private:
            void *owner;
            float freq;
            bool fixed;
            uint8_t envInterStep;
            ADSR env;
            Note note;
            SampleInfo sInfo;
            EnvState eState;
            uint8_t leftVol;
            uint8_t rightVol;
            uint8_t envLevel;
            // these values are always 1 frame behind in order to provide a smooth transition
            uint8_t processLeftVol;
            uint8_t processRightVol;
            uint8_t processEnvLevel;
    };
}
