#pragma once

#include <string>
#include <filesystem>

class OS {
public:
    static void CheckTerminal();
    static const std::filesystem::path GetLocalConfigDirectory();
    static const std::filesystem::path GetGlobalConfigDirectory();
};
