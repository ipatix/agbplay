#pragma once

#include <cstdint>

#include "SampleStructs.h"

namespace agbplay
{
    class CGBChannel
    {
        public: 
            CGBChannel();
            ~CGBChannel();
            virtual void Init(void *owner, CGBDef def, Note note, ADSR env);
            void *GetOwner();
            float GetFreq();
            void SetVol(uint8_t leftVol, uint8_t rightVol);
            ChnVol GetVol();
            CGBDef GetDef();
            uint8_t GetMidiKey();
            int8_t GetNoteLength();
            void Release();
            virtual void SetPitch(int16_t pitch) = 0;
            bool TickNote(); // returns true if note remains active
            EnvState GetState();
            virtual void StepEnvelope();
            void UpdateVolFade();
            const float *GetPat();
            uint32_t pos;
            float interPos;
        protected:
            void *owner;
            const float *pat;
            float freq;
            ADSR env;
            Note note;
            CGBDef def;
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

    class SquareChannel : public CGBChannel
    {
        public:
            SquareChannel();
            ~SquareChannel();

            void Init(void *owner, CGBDef def, Note note, ADSR env) override;
            void SetPitch(int16_t pitch);
    };

    class WaveChannel : public CGBChannel
    {
        public:
            WaveChannel();
            ~WaveChannel();

            void Init(void *owner, CGBDef def, Note note, ADSR env) override;
            void SetPitch(int16_t pitch) override;
        private:
            float waveBuffer[32];
    };

    class NoiseChannel : public CGBChannel
    {
        public:
            NoiseChannel();
            ~NoiseChannel();

            void Init(void *owner, CGBDef def, Note note, ADSR env) override;
            void SetPitch(int16_t pitch) override;
    };
}
