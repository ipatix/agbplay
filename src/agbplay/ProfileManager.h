#pragma once

#include "Profile.h"

#include <filesystem>
#include <functional>
#include <vector>
#include <list>
#include <string>

class Rom;

class ProfileManager {
public:
    void Reset();
    void LoadProfiles();
    void SaveProfiles();
    std::vector<std::reference_wrapper<Profile>> GetProfile(const Rom &rom);
    Profile &CreateProfile(const std::string &gameCode, size_t tableIdx);

private:
    void LoadProfileDir(const std::filesystem::path &filePath);
    void LoadProfile(const std::filesystem::path &filePath);
    void SaveProfile(Profile &profile);

    std::list<Profile> profiles;

    /* We use std::list instead of std::vector for profiles.
     * If a new profile is created and inserted, we do not want to invalidate references to existing profiles.
     * Maybe we should use shared_ptr instead... */
};
