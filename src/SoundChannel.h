#pragma once

#include <cstdint>

#include "SampleStructs.h"

namespace agbplay
{
    class SoundChannel
    {
        public:
            SoundChannel(uint8_t owner, SampleInfo sInfo, ADSR env, Note note, uint8_t vol, int8_t pan, int16_t pitch, bool fixed);
            ~SoundChannel();
            uint8_t GetOwner();
            float GetFreq();
            void SetVol(uint8_t vol, int8_t pan);
            //uint8_t GetVolL();
            //uint8_t GetVolR();
            ChnVol GetVol();
            uint8_t GetMidiKey();
            int8_t GetNoteLength();
            bool IsFixed();
            bool IsGS();
            void Release();
            void Kill();
            void SetPitch(int16_t pitch);
            bool TickNote(); // returns true if note remains active
            EnvState GetState();
            SampleInfo& GetInfo();
            uint8_t GetInterStep();
            void StepEnvelope();
            void UpdateVolFade();
            float interPos;
            uint32_t pos;
        private:
            uint8_t owner;
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
