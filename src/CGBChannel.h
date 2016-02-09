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
            void SetVol(uint8_t vol, int8_t pan);
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
            enum class Pan {
                LEFT, CENTER, RIGHT
            };
            void *owner;
            const float *pat;
            float freq;
            ADSR env;
            Note note;
            CGBDef def;
            EnvState eState;
            Pan pan;
            uint8_t envInterStep;
            uint8_t envLevel;
            uint8_t envPeak;
            uint8_t envSustain;
            // these values are always 1 frame behind in order to provide a smooth transition
            Pan fromPan;
            uint8_t fromEnvLevel;
    };

    class SquareChannel : public CGBChannel
    {
        public:
            SquareChannel();
            ~SquareChannel();

            void Init(void *owner, CGBDef def, Note note, ADSR env) override;
            void SetPitch(int16_t pitch) override;
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
            static uint8_t volLut[16];
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
