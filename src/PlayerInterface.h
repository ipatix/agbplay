#pragma once

#include <cstdint>
#include <vector>
#include <thread>
#include <memory>
#include <portaudio.h>

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
            PlayerInterface(TrackviewGUI *trackUI, long initSongPos);
            ~PlayerInterface();
            
            void LoadSong(long songPos);
            void Play();
            void Pause();
            void Stop();
            void SpeedDouble();
            void SpeedHalve();
            bool IsPlaying();
            void UpdateView();
            void ToggleMute(size_t index);
            void Mute(size_t index, bool mute);
            size_t GetMaxTracks() { return mutedTracks.size(); }
            void GetMasterVolLevels(float& left, float& right);
        private:
            void threadWorker();
            static int audioCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
                    const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags,
                    void *userData);

            void setupLoudnessCalcs();

            static const std::vector<PaHostApiTypeId> hostApiPriority;

            PaStream *audioStream;
            uint32_t speedFactor; // 64 = normal
            volatile enum class State : int { RESTART, PLAYING, PAUSED, TERMINATED, SHUTDOWN, THREAD_DELETED } playerState;
            std::unique_ptr<Sequence> seq;
            StreamGenerator *sg;
            TrackviewGUI *trackUI;
            Ringbuffer rBuf;

            LoudnessCalculator masterLoudness;
            std::vector<LoudnessCalculator> trackLoudness;
            std::vector<bool> mutedTracks;

            std::thread *playerThread;
    };
}
