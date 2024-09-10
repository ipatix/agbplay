#pragma once

#include "Profile.hpp"
#include "SonglistGUI.hpp"

class PlaylistGUI : public SonglistGUI
{
public:
    PlaylistGUI(
        uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos
    );
    ~PlaylistGUI() override;

    void AddSong(const Profile::PlaylistEntry &entry) override;
    void RemoveSong() override;
    void ClearSongs() override;
    const std::vector<bool> &GetTicked() const;
    void Tick();
    void Untick();
    void ToggleTick();
    void ToggleDrag();
    void UntickAll();
    bool IsDragging();
    void Leave() override;

private:
    void swapEntry(uint32_t a, uint32_t b);
    void update() override;
    void scrollDownNoUpdate() override;
    void scrollUpNoUpdate() override;

    std::vector<bool> ticked;
    bool dragging = false;
};
