#pragma once

#include <cstdint>
#include <vector>
#include <boost/thread.hpp>
#include <portaudio.h>

#include "Rom.h"
#include "TrackviewGUI.h"
#include "DisplayContainer.h"
#include "StreamGenerator.h"

namespace agbplay
{
    class PlayerInterface 
    {
        public:
            PlayerInterface(Rom& rom, TrackviewGUI *trackUI, long initSongPos, EnginePars pars);
            ~PlayerInterface();
            
            void LoadSong(long songPos, uint8_t trackLimit);
            void Play();
            void Pause();
            void Stop();
        private:
            void threadWorker();

            PaStream *audioStream;
            EnginePars pars;
            volatile enum class State : int { RESTART, PLAYING, PAUSED, TERMINATED, SHUTDOWN, THREAD_DELETED } playerState;
            Sequence seq;
            Rom& rom;
            TrackviewGUI *trackUI;

            boost::thread *playerThread;
    };
}
