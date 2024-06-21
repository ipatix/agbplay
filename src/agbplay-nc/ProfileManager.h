#pragma once

#include "Profile.h"

#include <filesystem>
#include <functional>
#include <vector>
#include <list>

class Rom;

class ProfileManager {
public:
    void SetPath(const std::filesystem::path &baseDir);
    void Reset();
    void LoadProfiles();
    void SaveProfiles();
    std::vector<std::reference_wrapper<Profile>> GetProfile(const Rom &rom);

private:
    void LoadProfilesDir(const std::filesystem::path &filePath);
    void LoadProfile(const std::filesystem::path &filePath);
    void SaveProfile(Profile &profile);

    std::filesystem::path baseDir;
    std::list<Profile> profiles;

    /* We use std::list instead of std::vector for profiles.
     * If a new profile is created and inserted, we do not want to invalidate references to existing profiles. */
};
