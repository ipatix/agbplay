#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "CursesWin.h"

using namespace std;

namespace agbplay {
    class SongEntry {
        public:
            SongEntry(string name, uint16_t uid);
            ~SongEntry();

            uint16_t GetUID();
            string name;
        private:
            uint16_t uid;
    };

    class SonglistGUI : public CursesWin {
        public:
            SonglistGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos, bool upd);
            virtual ~SonglistGUI();
            void Resize(uint32_t height, uint32_t width,
                    uint32_t yPos, uint32_t xPos) override;

            virtual void AddSong(SongEntry song);
            virtual void RemoveSong();
            virtual void ClearSongs();
            SongEntry GetSong() throw();
            void Enter();
            virtual void Leave();
            void ScrollDown();
            void ScrollUp();
            void PageDown();
            void PageUp();
        protected:
            virtual void scrollDownNoUpdate();
            virtual void scrollUpNoUpdate();
            void update() override;
            void checkDimensions(uint32_t height, uint32_t width);

            vector<SongEntry> *songlist;
            uint32_t viewPos;
            uint32_t cursorPos;
            uint32_t contentHeight;
            uint32_t contentWidth;
            bool cursorVisible;
    };
}
