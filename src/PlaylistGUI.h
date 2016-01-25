#pragma once

#include "SonglistGUI.h"

namespace agbplay {
    class PlaylistGUI : public SonglistGUI {
        public:
            PlaylistGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos, std::string gameCode);
            ~PlaylistGUI();

            void AddSong(SongEntry) override;
            void RemoveSong() override;
            void ClearSongs() override;
            void Leave() override;
            void Tick();
            void Untick();
            void ToggleTick();
            void ToggleDrag();
        private:
            void swapEntry(uint32_t a, uint32_t b);
            void update() override;
            void scrollDownNoUpdate() override;
            void scrollUpNoUpdate() override;

            vector<bool> *ticked;
            std::string gameCode;
            bool dragging;
    };
}
