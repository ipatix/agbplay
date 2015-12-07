#pragma once

#include <cstdint>
#include <vector>

#include "Rom.h"
#include "TrackviewGUI.h"
#include "DisplayContainer.h"
#include "SoundData.h"

namespace agbplay 
{
    struct EnginePars
    {
        EnginePars(uint8_t vol, uint8_t rev, uint8_t freq);

        uint8_t vol;
        uint8_t rev;
        uint8_t freq;
    };

    class PlayerModule 
    {
        public:
            PlayerModule(Rom& rrom, TrackviewGUI *trackUI, long initSongPos, EnginePars pars);
            ~PlayerModule();
            
            void LoadSong(long songPos, uint8_t trackLimit);
            void Play();
            void Pause();
            void Stop();
        private:
            const std::vector<uint32_t> freqLut = {
                5734, 7884, 10512, 13379,
                15768, 18157, 21024, 26758,
                31536, 36314, 40137, 42048
            };

            uint32_t dSoundFreq;
            uint8_t dSoundVol;
            uint8_t dSoundRev;

            bool isPlaying;
            bool isPaused;
            Sequence seq;
            Rom& rom;
            TrackviewGUI *trackUI;
    };
}
