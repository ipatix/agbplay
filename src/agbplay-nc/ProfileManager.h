#pragma once

#include "Profile.h"

#include <filesystem>
#include <vector>

class ProfileManager {
public:
    void SetPath(const std::filesystem::path &baseDir);
    void Reset();
    void LoadProfiles();
    void SaveProfiles();

private:
    void LoadProfilesDir(const std::filesystem::path &filePath);
    void LoadProfile(const std::filesystem::path &filePath);
    void SaveProfile(Profile &profile);

    std::filesystem::path baseDir;
    std::vector<Profile> profiles;
};
