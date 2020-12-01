#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "CursesWin.h"
#include "SongEntry.h"

class SonglistGUI : public CursesWin 
{
public:
    SonglistGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos, bool upd);
    virtual ~SonglistGUI();
    void Resize(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos) override;
    virtual void AddSong(SongEntry song);
    virtual void RemoveSong();
    virtual void ClearSongs();
    virtual SongEntry& GetSong();
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
    uint32_t viewPos;
    uint32_t cursorPos;
    uint32_t contentHeight;
    uint32_t contentWidth;
    bool cursorVisible;
private:
    std::vector<SongEntry> *songlist;
};
