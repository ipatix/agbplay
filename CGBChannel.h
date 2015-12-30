#pragma once

#include <cstdint>

#include "SampleStructs.h"

namespace agbplay
{
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
            void StepEnvelope();
            float interPos;
        private:
            void *owner;
            float freq;
            uint8_t envInterStep;
            ADSR env;
            Note note;
            CGBDef def;
            CGBType cType;
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
