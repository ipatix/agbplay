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
            //uint8_t GetVolL();
            //uint8_t GetVolR();
            ChnVol GetVol();
            uint8_t GetMidiKey();
            bool IsFixed();
            bool IsGS();
            void Release();
            void Kill();
            void SetPitch(int16_t pitch);
            bool TickNote(); // returns true if note remains active
            EnvState GetState();
            SampleInfo& GetInfo();
            void StepEnvelope();
            void UpdateVolFade();
            float interPos;
            uint32_t pos;
        private:
            void *owner;
            float freq;
            bool fixed;
            bool isGS;
            ADSR env;
            Note note;
            SampleInfo sInfo;
            EnvState eState;
            uint8_t envInterStep;
            uint8_t leftVol;
            uint8_t rightVol;
            uint8_t envLevel;
            // these values are always 1 frame behind in order to provide a smooth transition
            uint8_t fromLeftVol;
            uint8_t fromRightVol;
            uint8_t fromEnvLevel;
    };
}
