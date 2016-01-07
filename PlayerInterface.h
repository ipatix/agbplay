#pragma once

#include <cstdint>
#include <vector>
#include <boost/thread.hpp>

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
            void Unpause();
            void Stop();
        private:
            static const std::vector<uint32_t> freqLut;

            uint32_t dSoundFreq;
            uint8_t dSoundVol;
            uint8_t dSoundRev;

            // RESTART = 
            enum class State : int { RESTART, PLAYING, PAUSED, STOPPING, STOPPED } playerState;
            Sequence seq;
            Rom& rom;
            TrackviewGUI *trackUI;

            boost::thread playerThread;
    };
}
