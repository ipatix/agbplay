#pragma once

#include <filesystem>
#include <string>

namespace OS
{
    void LowerThreadPriority();
    void CheckTerminal();
    const std::filesystem::path GetMusicDirectory();
    const std::filesystem::path GetLocalConfigDirectory();
    const std::filesystem::path GetGlobalConfigDirectory();
};    // namespace OS
