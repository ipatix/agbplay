#include "ProfileManager.h"

#include <fstream>
#include <limits>

#include <fmt/core.h>
#include <nlohmann/json.hpp>

#include "Xcept.h"
#include "Rom.h"
#include "OS.h"

void ProfileManager::Reset()
{
    profiles.clear();
}

void ProfileManager::LoadProfiles()
{
    const auto configDir = OS::GetLocalConfigDirectory();
    LoadProfileDir(configDir / "agbplay" / "profiles");
    LoadProfileDir(configDir / "agbplay" / "profiles-user");
}

void ProfileManager::LoadProfileDir(const std::filesystem::path &dir)
{
    if (!std::filesystem::is_directory(dir))
        throw std::invalid_argument(fmt::format("ProfileManager: Profile directory is not a directory: {}", dir.string()));

    for (const auto &dirEntry : std::filesystem::directory_iterator(dir)) {
        if (dirEntry.is_directory())
            LoadProfileDir(dirEntry.path());
        else if (dirEntry.path().extension() == ".json")
            LoadProfile(dirEntry.path());
    }
}

void ProfileManager::SaveProfiles()
{
    for (Profile &profile : profiles) {
        SaveProfile(profile);
    }
}

std::vector<std::reference_wrapper<Profile>> ProfileManager::GetProfile(const Rom &rom)
{
    /* Find all profiles which match the ROM passed.
     * If only a single match is found, return it.
     * If multiples are found and if there is a strong match (e.g. magic bytes), return it.
     * If multiples are found and none is preferred, return them all */
    const auto gameCode = rom.GetROMCode();

    /* find all matching game codes */
    std::vector<std::reference_wrapper<Profile>> profilesWithGameCode;

    for (Profile &profile : profiles) {
        for (const std::string &gameCodeToCheck : profile.gameMatch.gameCodes) {
            if (gameCodeToCheck == gameCode) {
                profilesWithGameCode.emplace_back(profile);
                break;
            }
        }
    }

    /* find all profiles with magic bytes */
    std::vector<std::reference_wrapper<Profile>> profilesWithMagicBytes;

    for (Profile &profile : profilesWithGameCode) {
        if (profile.gameMatch.magicBytes.size() != 0)
            profilesWithMagicBytes.emplace_back(profile);
    }

    if (profilesWithMagicBytes.size() == 0)
        return profilesWithGameCode;

    /* find all profiles with matching magic bytes */
    std::vector<std::reference_wrapper<Profile>> profilesToReturn;

    // probably more complex than necessary, but I want to try a bloom filter today!
    std::vector<uint8_t> bloomFilter;
    for (const Profile &profile : profilesWithMagicBytes) {
        const auto &mb = profile.gameMatch.magicBytes;
        if (mb.size() > bloomFilter.size())
            bloomFilter.resize(mb.size(), 0);

        for (size_t i = 0; i < mb.size(); i++)
            bloomFilter.at(i) |= mb.at(i);
    }

    for (size_t romIdx = 0; romIdx < rom.Size(); romIdx++) {
        bool filterMatch = true;
        const size_t clipSize = rom.Size() - romIdx;
        for (size_t filtIdx = 0; filtIdx < std::min(bloomFilter.size(), clipSize); filtIdx++) {
            const auto data = rom.ReadU8(romIdx + filtIdx);
            if ((data & bloomFilter.at(filtIdx)) != data) {
                filterMatch = false;
                break;
            }
        }

        if (filterMatch) [[unlikely]] {
            /* if the bloom filter matched, now check which one actually matched */
            auto &pwmb = profilesWithMagicBytes;
            auto filterFunc = [&](Profile &profile){
                const auto &mb = profile.gameMatch.magicBytes;
                if (romIdx + mb.size() > rom.Size())
                    return false;
                bool exactMatch = true;
                for (size_t filtIdx = 0; filtIdx < mb.size(); filtIdx++) {
                    if (rom.ReadU8(romIdx + filtIdx) != mb.at(filtIdx)) {
                        exactMatch = false;
                        break;
                    }
                }
                if (exactMatch) {
                    profilesToReturn.emplace_back(profile);
                    return true;
                }
                return false;
            };

            pwmb.erase(std::remove_if(pwmb.begin(), pwmb.end(), filterFunc), pwmb.end());
        }
    }

    return profilesToReturn;
}

Profile &ProfileManager::CreateProfile(const std::string &gameCode)
{
    auto &profile = profiles.emplace_back();
    profile.gameMatch.gameCodes.emplace_back(gameCode);
    profile.path = OS::GetLocalConfigDirectory() / "agbplay" / "profiles" / (gameCode + ".json");
    profile.dirty = true;
    return profile;
}

void ProfileManager::LoadProfile(const std::filesystem::path &filePath)
{
    // TODO issue warnings instead of ignoring them or throwing errors
    using nlohmann::json;

    std::ifstream fileStream(filePath);
    if (!fileStream.is_open()) {
        // perhaps it's better to just issue a warning instead of failing
        const std::string err = strerror(errno);
        throw Xcept("Failed to open file: {}, {}", filePath.string(), err);
    }

    json j = json::parse(fileStream);
    fileStream.close();

    Profile p;

    p.path = filePath;

    /* load playlist */
    if (j.contains("playlist") && j["playlist"].is_array()) {
        for (const auto &entry : j["playlist"]) {
            if (!entry.contains("name") || !entry["name"].is_string())
                continue;
            if (!entry.contains("id") || !entry["id"].is_number())
                continue;

            const std::string &name = entry["name"];
            if (name.size() == 0)
                continue;
            const int64_t id = entry["id"];
            if (id < std::numeric_limits<uint16_t>::min() || id > std::numeric_limits<uint16_t>::max())
                continue;

            p.playlist.emplace_back(name, id);
        }
    }

    /* load song table info */
    if (j.contains("songTableInfo") && j.is_object()) {
        const auto &sti= j["songTableInfo"];

        // TODO range checks
        if (sti.contains("pos") && sti["pos"].is_number()) {
            p.songTableInfoConfig.pos = sti["pos"];
        } else {
            p.songTableInfoConfig.pos = SongTableInfo::POS_AUTO;
        }

        if (sti.contains("count") && sti["count"].is_number()) {
            p.songTableInfoConfig.count = sti["count"];
        } else {
            p.songTableInfoConfig.count = SongTableInfo::COUNT_AUTO;
        }

        if (p.songTableInfoConfig.pos != SongTableInfo::POS_AUTO) {
            p.songTableInfoConfig.tableIdx = 0;
        } else if (sti.contains("tableIdx") && sti["tableIdx"].is_number()) {
            p.songTableInfoConfig.tableIdx = sti["tableIdx"];
        } else {
            p.songTableInfoConfig.tableIdx = 0;
        }
    } else {
        p.songTableInfoConfig.pos = SongTableInfo::POS_AUTO;
        p.songTableInfoConfig.count = SongTableInfo::COUNT_AUTO;
        p.songTableInfoConfig.tableIdx = 0;
    }

    /* load player table config */
    if (j.contains("playerTable") && j["playerTable"].is_array()) {
        for (const auto &entry : j["playerTable"]) {
            if (!entry.is_object())
                break;
            PlayerInfo pi;
            if (!entry.contains("maxTracks") || !entry["maxTracks"].is_number())
                break;
            pi.maxTracks = entry["maxTracks"];
            if (!entry.contains("usePriority") || !entry["usePriority"].is_boolean())
                pi.usePriority = false;
            else
                pi.usePriority = entry["usePriority"];
            p.playerTableConfig.emplace_back(pi);
        }
    }

    /* load mp2k sound mode */
    if (j.contains("mp2kSoundMode") && j["mp2kSoundMode"].is_object()) {
        // p.mp2kSoundModeConfig is default initialized with 'auto'
        const auto &sm = j["mp2kSoundMode"];
        if (sm.contains("vol") && sm["vol"].is_number())
            p.mp2kSoundModeConfig.vol = static_cast<uint8_t>(std::clamp<int64_t>(sm["vol"], 0, 15));
        if (sm.contains("rev") && sm["rev"].is_number())
            p.mp2kSoundModeConfig.rev = static_cast<uint8_t>(std::clamp<int64_t>(sm["rev"], 0, 255));
        if (sm.contains("freq") && sm["freq"].is_number())
            p.mp2kSoundModeConfig.freq = static_cast<uint8_t>(std::clamp<int64_t>(sm["freq"], 1, 12));
        if (sm.contains("maxChannels") && sm["maxChannels"].is_number())
            p.mp2kSoundModeConfig.maxChannels = static_cast<uint8_t>(std::clamp<int64_t>(sm["maxChannels"], 1, 12));
        if (sm.contains("dacConfig") && sm["dacConfig"].is_number())
            p.mp2kSoundModeConfig.dacConfig = static_cast<uint8_t>(std::clamp<int64_t>(sm["dacConfig"], 8, 11));
    }

    /* load agbplay sound mode */
    if (j.contains("agbplaySoundMode") && j["agbplaySoundMode"].is_object()) {
        // p.agbplaySoundMode is default initialized
        const auto &sm = j["agbplaySoundMode"];
        if (sm.contains("resamplerTypeNormal") && sm["resamplerTypeNormal"].is_string())
            p.agbplaySoundMode.resamplerTypeNormal = str2res(sm["resamplerTypeNormal"]);
        if (sm.contains("resamplerTypeFixed") && sm["resamplerTypeFixed"].is_string())
            p.agbplaySoundMode.resamplerTypeFixed = str2res(sm["resamplerTypeFixed"]);
        if (sm.contains("reverbType") && sm["reverbType"].is_string())
            p.agbplaySoundMode.reverbType = str2rev(sm["reverbType"]);
        if (sm.contains("cgbPolyphony") && sm["cgbPolyphony"].is_string())
            p.agbplaySoundMode.cgbPolyphony = str2cgbPoly(sm["cgbPolyphony"]);
        if (sm.contains("dmaBufferLen") && sm["dmaBufferLen"].is_number())
            p.agbplaySoundMode.dmaBufferLen = sm["dmaBufferLen"];
        if (sm.contains("maxLoops") && sm["maxLoops"].is_number())
            p.agbplaySoundMode.maxLoops = sm["maxLoops"];
        if (sm.contains("padSilenceSecondsStart") && sm["padSilenceSecondsStart"].is_number())
            p.agbplaySoundMode.padSilenceSecondsStart = sm["padSilenceSecondsStart"];
        if (sm.contains("padSilenceSecondsEnd") && sm["padSilenceSecondsEnd"].is_number())
            p.agbplaySoundMode.padSilenceSecondsEnd = sm["padSilenceSecondsEnd"];
        if (sm.contains("accurateCh3Quantization") && sm["accurateCh3Quantization"].is_boolean())
            p.agbplaySoundMode.accurateCh3Quantization = sm["accurateCh3Quantization"];
        if (sm.contains("accurateCh3Volume") && sm["accurateCh3Volume"].is_boolean())
            p.agbplaySoundMode.accurateCh3Volume = sm["accurateCh3Volume"];
        if (sm.contains("emulateCgbSustainBug") && sm["emulateCgbSustainBug"].is_boolean())
            p.agbplaySoundMode.emulateCgbSustainBug = sm["emulateCgbSustainBug"];
    }

    /* load game match */
    if (j.contains("gameMatch") && j["gameMatch"].is_object()) {
        const auto &gm = j["gameMatch"];
        if (!gm.contains("gameCodes") || !gm["gameCodes"].is_array())
            throw Xcept("Cannot parse profile: {}, gameCodes does not exist or is not an array", filePath.string());

        for (const auto &gameCode : gm["gameCodes"]) {
            if (!gameCode.is_string())
                continue;
            if (std::string(gameCode).size() != 4)
                continue;
            p.gameMatch.gameCodes.emplace_back(std::string(gameCode));
        }

        if (p.gameMatch.gameCodes.size() == 0)
            throw Xcept("Cannot parse profile: {}, the profile must at least match one game code", filePath.string());

        if (gm.contains("magicBytes") && gm["magicBytes"].is_array()) {
            for (const auto &byte : gm["magicBytes"]) {
                if (!byte.is_number())
                    p.gameMatch.magicBytes.emplace_back(byte);
            }
        }
    } else {
        throw Xcept("Cannot parse profile: {}, missing field: gameMatch", filePath.string());
    }

    /* load description */
    if (j.contains("description") && j["description"].is_string()) {
        p.description = j["description"];
    }


    profiles.push_back(std::move(p));
}

void ProfileManager::SaveProfile(Profile &p)
{
    if (!p.dirty)
        return;

    using nlohmann::json;
    json j;

    /* save playlist */
    if (p.playlist.size() != 0) {
        json jpl = json::array();
        for (const auto &entry : p.playlist) {
            json jen;
            jen["name"] = entry.name;
            jen["id"] = entry.id;
            jpl.push_back(std::move(jen));
        }
        j["playlist"] = std::move(jpl);
    }

    /* save song table info */
    json jsti;
    if (p.songTableInfoConfig.count != SongTableInfo::COUNT_AUTO)
        jsti["count"] = p.songTableInfoConfig.count;
    if (p.songTableInfoConfig.pos == SongTableInfo::POS_AUTO) {
        jsti["tableIdx"] = p.songTableInfoConfig.tableIdx;
    } else {
        jsti["pos"] = p.songTableInfoConfig.pos;
    }
    j["songTableInfo"] = std::move(jsti);

    /* save player table config */
    if (p.playerTableConfig.size() != 0) {
        json jpti = json::array();;

        for (const auto &player : p.playerTableConfig) {
            json jpi;
            jpi["maxTracks"] = player.maxTracks;
            jpi["usePriority"] = player.usePriority;
            jpti.push_back(std::move(jpi));
        }

        jpti["playerTable"] = std::move(jpti);
    }

    /* save mp2k sound mode */
    json jmsm = json::object();
    if (p.mp2kSoundModeConfig.vol != MP2KSoundMode::VOL_AUTO)
        jmsm["vol"] = p.mp2kSoundModeConfig.vol;
    if (p.mp2kSoundModeConfig.rev != MP2KSoundMode::REV_AUTO)
        jmsm["rev"] = p.mp2kSoundModeConfig.rev;
    if (p.mp2kSoundModeConfig.freq != MP2KSoundMode::FREQ_AUTO)
        jmsm["freq"] = p.mp2kSoundModeConfig.freq;
    if (p.mp2kSoundModeConfig.maxChannels != MP2KSoundMode::CHN_AUTO)
        jmsm["maxChannels"] = p.mp2kSoundModeConfig.maxChannels;
    if (p.mp2kSoundModeConfig.dacConfig != MP2KSoundMode::DAC_AUTO)
        jmsm["dacConfig"] = p.mp2kSoundModeConfig.dacConfig;
    if (jmsm.size() != 0)
        j["mp2kSoundMode"] = std::move(jmsm);

    /* save agbplay sound mode */
    json jasm = json::object();
    jasm["resamplerTypeNormal"] = res2str(p.agbplaySoundMode.resamplerTypeNormal);
    jasm["resamplerTypeFixed"] = res2str(p.agbplaySoundMode.resamplerTypeFixed);
    jasm["reverbType"] = rev2str(p.agbplaySoundMode.reverbType);
    jasm["cgbPolyphony"] = cgbPoly2str(p.agbplaySoundMode.cgbPolyphony);
    jasm["dmaBufferLen"] = p.agbplaySoundMode.dmaBufferLen;
    jasm["maxLoops"] = p.agbplaySoundMode.maxLoops;
    jasm["padSilenceSecondsStart"] = p.agbplaySoundMode.padSilenceSecondsStart;
    jasm["padSilenceSecondsEnd"] = p.agbplaySoundMode.padSilenceSecondsEnd;
    jasm["accurateCh3Quantization"] = p.agbplaySoundMode.accurateCh3Quantization;
    jasm["accurateCh3Volume"] = p.agbplaySoundMode.accurateCh3Volume;
    jasm["emulateCgbSustainBug"] = p.agbplaySoundMode.emulateCgbSustainBug;
    j["agbplaySoundMode"] = std::move(jasm);

    /* save game match */
    json jgm = json::object();
    json jgc = json::array();
    assert(p.gameMatch.gameCodes.size() >= 1);
    for (const std::string &gameCode : p.gameMatch.gameCodes)
        jgc.push_back(gameCode);
    jgm["gameCodes"] = std::move(jgc);
    if (p.gameMatch.magicBytes.size() != 0) {
        json jmb = json::array();
        for (const uint8_t b : p.gameMatch.magicBytes)
            jmb.push_back(b);
        jgm["magicBytes"] = std::move(jmb);
    }
    j["gameMatch"] = jgm;

    /* save description */
    if (p.description.size() > 0)
        j["description"] = p.description;

    /* save JSON to disk */
    std::ofstream fileStream(p.path);
    if (!fileStream.is_open()) {
        // TODO we very likely do not want to crash in case of write failure.
        // Better give the user an option to try again (i.e. after permissions are fixed or disk space freed).
        const std::string err = strerror(errno);
        throw Xcept("Failed to save file: {}, {}", p.path.string(), err);
    }
    fileStream << std::setw(2) << j << std::endl;
    fileStream.close();

    p.dirty = false;
}
