#pragma once

#include <cstdint>
#include <vector>
#include <boost/thread.hpp>
#include <portaudio.h>

#include "Rom.h"
#include "TrackviewGUI.h"
#include "DisplayContainer.h"
#include "StreamGenerator.h"
#include "Constants.h"
#include "GameConfig.h"
#include "Ringbuffer.h"

namespace agbplay
{
    class PlayerInterface 
    {
        public:
            PlayerInterface(Rom& rom, TrackviewGUI *trackUI, long initSongPos, GameConfig& _gameCfg);
            ~PlayerInterface();
            
            void LoadSong(long songPos);
            void Play();
            void Pause();
            void Stop();
            void SpeedDouble();
            void SpeedHalve();
            bool IsPlaying();
            void UpdateView();
            void GetVolLevels(float& left, float& right);
        private:
            void threadWorker();
            static int audioCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
                    const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags,
                    void *userData);
            void writeMaxLevels(float *buffer, size_t nBlocks);

            float avgVolLeft;
            float avgVolRight;
            float avgVolLeftSq;
            float avgVolRightSq;

            PaStream *audioStream;
            uint32_t speedFactor; // 64 = normal
            volatile enum class State : int { RESTART, PLAYING, PAUSED, TERMINATED, SHUTDOWN, THREAD_DELETED } playerState;
            Rom& rom;
            GameConfig& gameCfg;
            Sequence seq;
            StreamGenerator *sg;
            TrackviewGUI *trackUI;
            Ringbuffer rBuf;

            boost::thread *playerThread;
    };
}
