#pragma once

#include "Profile.h"
#include "MP2KScanner.h"

#include <filesystem>
#include <functional>
#include <vector>
#include <string>
#include <memory>

class Rom;

class ProfileManager {
public:
    void Reset();
    void LoadProfiles();
    void SaveProfiles();
    std::vector<std::shared_ptr<Profile>> GetProfiles(const Rom &rom, const std::vector<MP2KScanner::Result> scanResults);
    std::shared_ptr<Profile> &CreateProfile(const std::string &gameCode, size_t tableIdx);

private:
    void ApplyScanResultsToProfiles(const Rom &rom, std::vector<std::shared_ptr<Profile>> &profiles, const std::vector<MP2KScanner::Result> scanResults);
    void LoadProfileDir(const std::filesystem::path &filePath);
    void LoadProfile(const std::filesystem::path &filePath);
    void SaveProfile(std::shared_ptr<Profile> &profile);

    std::vector<std::shared_ptr<Profile>> profiles;
};
