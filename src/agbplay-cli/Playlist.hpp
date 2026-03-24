#pragma once

#include <string>

namespace CLI
{
    void PlaylistList();
    void PlaylistCount();
    void PlaylistSongAdd(const std::string &name, const std::string &songId, const std::string &playlistIdx);
    void PlaylistSongRemove(const std::string &playlistIdx);
}
