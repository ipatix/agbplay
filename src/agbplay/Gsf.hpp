#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace Gsf
{
    bool GetRomData(std::span<const uint8_t> gsfData, std::vector<uint8_t> &resultRomData);
    void GetSongInfo(std::span<const uint8_t> gsfData, std::string &name, uint16_t &id);
    std::string GuessGameCodeFromPath(const std::filesystem::path &p);
}    // namespace Gsf
