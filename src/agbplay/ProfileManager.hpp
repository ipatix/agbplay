#pragma once

#include "MP2KScanner.hpp"
#include "Profile.hpp"

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class Rom;

class ProfileManager
{
public:
    void Reset();
    void LoadProfiles();
    void SaveProfiles();
    static std::filesystem::path ProfileUserPath();
    std::vector<std::shared_ptr<Profile>> GetProfiles(const Rom &rom);
    std::vector<std::shared_ptr<Profile>> &GetAllProfiles();
    std::shared_ptr<Profile> &CreateProfile(const std::string &gameCode, size_t tableIdx);
    static void SetStandardPath(Profile &profile, const std::string &gameCode, size_t tableIdx);

private:
    void ScanRomToProfiles(
        const Rom &rom,
        std::vector<std::shared_ptr<Profile>> &profiles
    );
    void LoadProfileDir(const std::filesystem::path &filePath);
    void LoadProfile(const std::filesystem::path &filePath);
    void SaveProfile(std::shared_ptr<Profile> &profile);

    std::vector<std::shared_ptr<Profile>> profiles;
};
