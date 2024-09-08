#pragma once

#include "Profile.hpp"
#include "SonglistGUI.hpp"

class PlaylistGUI : public SonglistGUI
{
public:
    PlaylistGUI(
        uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos, std::vector<Profile::PlaylistEntry> &playlist
    );
    ~PlaylistGUI() override;

    void AddSong(const Profile::PlaylistEntry &entry) override;
    void RemoveSong() override;
    void ClearSongs() override;
    Profile::PlaylistEntry *GetSong() override;
    const std::vector<bool> &GetTicked() const;
    void Leave() override;
    void Tick();
    void Untick();
    void ToggleTick();
    void ToggleDrag();
    void UntickAll();
    bool IsDragging();

private:
    void swapEntry(uint32_t a, uint32_t b);
    void update() override;
    void scrollDownNoUpdate() override;
    void scrollUpNoUpdate() override;

    std::vector<Profile::PlaylistEntry> &playlist;
    std::vector<bool> ticked;
    bool dragging = false;
};
