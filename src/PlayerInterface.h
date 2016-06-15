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
#include "LoudnessCalculator.h"

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
            void GetMasterVolLevels(float& left, float& right);
        private:
            void threadWorker();
            static int audioCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
                    const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags,
                    void *userData);

            void setupLoudnessCalcs();

            PaStream *audioStream;
            uint32_t speedFactor; // 64 = normal
            volatile enum class State : int { RESTART, PLAYING, PAUSED, TERMINATED, SHUTDOWN, THREAD_DELETED } playerState;
            Rom& rom;
            GameConfig& gameCfg;
            Sequence seq;
            StreamGenerator *sg;
            TrackviewGUI *trackUI;
            Ringbuffer rBuf;

            LoudnessCalculator masterLoudness;
            std::vector<LoudnessCalculator> trackLoudness;

            boost::thread *playerThread;
    };
}
