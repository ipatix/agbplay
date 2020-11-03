#pragma once

#include <string>
#include <filesystem>

class OS {
public:
    static const std::filesystem::path GetLocalConfigDirectory();
    static const std::filesystem::path GetGlobalConfigDirectory();
};
