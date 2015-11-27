#pragma once

#include <cstdint>

#include "Rom.h"
#include "TrackviewGUI.h"
#include "DisplayContainer.h"
#include "SoundData.h"

namespace agbplay 
{
    class PlayerModule 
    {
        public:
            PlayerModule(Rom& rrom, TrackviewGUI *trackUI, long initSongPos);
            ~PlayerModule();
            
            void LoadSong(long songPos);
            void Play();
            void Pause();
            void Stop();
        private:           
            bool isPlaying;
            bool isPaused;
            Sequence seq;
            Rom& rom;
            TrackviewGUI *trackUI;
    };
}
