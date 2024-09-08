#pragma once

#include "CursesWin.hpp"
#include "Profile.hpp"

#include <cstdint>
#include <string>
#include <vector>

class SonglistGUI : public CursesWin
{
public:
    SonglistGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos, bool upd);
    virtual ~SonglistGUI() override;
    void Resize(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos) override;
    virtual void AddSong(const Profile::PlaylistEntry &entry);
    virtual void RemoveSong();
    virtual void ClearSongs();
    virtual Profile::PlaylistEntry *GetSong();
    void Enter();
    virtual void Leave();
    void ScrollDown();
    void ScrollUp();
    void PageDown();
    void PageUp();
    bool IsLast() const;

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
    std::vector<Profile::PlaylistEntry> songlist;
};
