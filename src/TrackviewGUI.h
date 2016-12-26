#pragma once

#include <vector>
#include <string>

#include "CursesWin.h"
#include "SoundData.h"
#include "DisplayContainer.h"

namespace agbplay 
{
    class TrackviewGUI : public CursesWin 
    {
        public:
            TrackviewGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos);
            ~TrackviewGUI();

            void Resize(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos) override;
            void SetState(const Sequence& seq, const float *vols);
            void SetTitle(const std::string& name);
            void Enter();
            void Leave();
            void PageDown();
            void PageUp();
            void ScrollDown();
            void ScrollUp();
            void ForceUpdate();
            uint32_t GetCursorLoc() const { return cursorPos; }
        private:
            void update() override;
            void scrollDownNoUpdate();
            void scrollUpNoUpdate();

            DisplayContainer disp; std::string songName; 
            uint32_t cursorPos;
            bool cursorVisible;
            static const std::vector<const char *> noteNames;
    }; // end TrackviewGUI
} // end namespace agbplay
