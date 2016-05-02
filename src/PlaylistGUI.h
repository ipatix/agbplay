#pragma once

#include "SonglistGUI.h"
#include "GameConfig.h"

namespace agbplay {
    class PlaylistGUI : public SonglistGUI {
        public:
            PlaylistGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos, GameConfig& _thisGameConfig);
            ~PlaylistGUI();

            void AddSong(SongEntry) override;
            void RemoveSong() override;
            void ClearSongs() override;
            SongEntry& GetSong() override;
            std::vector<bool>& GetTicked();
            void Leave() override;
            void Tick();
            void Untick();
            void ToggleTick();
            void ToggleDrag();
            bool IsDragging();
        private:
            void swapEntry(uint32_t a, uint32_t b);
            void update() override;
            void scrollDownNoUpdate() override;
            void scrollUpNoUpdate() override;

            GameConfig& thisGameConfig;

            std::vector<bool> *ticked;
            std::string gameCode;
            bool dragging;
    };
}
