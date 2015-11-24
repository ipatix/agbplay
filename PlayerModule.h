#pragma once

#include <cstdint>

#include "Rom.h"
#include "TrackviewGUI.h"
#include "DisplayContainer.h"

namespace agbplay {
    class PlayerModule {
        public:
            PlayerModule(Rom& rrom, TrackviewGUI *trackUI);
            ~PlayerModule();
            
            void LoadSong(uint16_t songNum);
            void Play();
            void Pause();
            void Stop();

        private:
            
            uint16_t loadedSong;
            bool isPlaying;
            bool isPaused;

            Rom& rom;
            TrackviewGUI *trackUI;

            DisplayContainer cont;
    };
}
