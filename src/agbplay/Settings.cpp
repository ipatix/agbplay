#include "Settings.hpp"

#include "OS.hpp"
#include "Xcept.hpp"

#include <cerrno>
#include <fstream>
#include <nlohmann/json.hpp>

static const std::filesystem::path CONFIG_PATH = OS::GetLocalConfigDirectory() / "agbplay" / "config.json";
static const uint32_t DEFAULT_SAMPLERATE = 48000;
static const bool DEFAULT_QUICK_EXPORT_ASK = true;

void Settings::Load()
{
    using nlohmann::json;

    std::ifstream fileStream(CONFIG_PATH);
    if (!fileStream.is_open()) {
        /* Not sure if errno checking is portable. */
        if (errno == ENOENT) {
            /* If the config does not exist, this is a normal use case and we silently initialize a standard config. */
            playbackSampleRate = DEFAULT_SAMPLERATE;
            exportSampleRate = DEFAULT_SAMPLERATE;
            exportQuickExportDirectory = OS::GetMusicDirectory();
            exportQuickExportAsk = DEFAULT_QUICK_EXPORT_ASK;
            return;
        }
        const std::string err = strerror(errno);
        throw Xcept("Failed to open file: {}, {}", CONFIG_PATH.string(), err);
    }

    json j = json::parse(fileStream);
    fileStream.close();

    if (j.contains("playbackSampleRate") && j["playbackSampleRate"].is_number()) {
        playbackSampleRate = j["playbackSampleRate"];
    } else {
        playbackSampleRate = DEFAULT_SAMPLERATE;
    }

    if (j.contains("exportSampleRate") && j["exportSampleRate"].is_number()) {
        exportSampleRate = j["exportSampleRate"];
    } else {
        exportSampleRate = DEFAULT_SAMPLERATE;
    }

    if (j.contains("exportQuickExportDirectory") && j["exportQuickExportDirectory"].is_string()) {
        exportQuickExportDirectory =
            reinterpret_cast<const char8_t *>(std::string(j["exportQuickExportDirectory"]).c_str());
    } else {
        exportQuickExportDirectory = OS::GetMusicDirectory();
    }

    if (j.contains("exportQuickExportAsk") && j["exportQuickExportAsk"].is_boolean()) {
        exportQuickExportAsk = j["exportQuickExportAsk"];
    } else {
        exportQuickExportAsk = DEFAULT_QUICK_EXPORT_ASK;
    }
}

void Settings::Save()
{
    using nlohmann::json;

    json j;
    j["playbackSampleRate"] = playbackSampleRate;
    j["exportSampleRate"] = exportSampleRate;
    j["exportQuickExportDirectory"] = exportQuickExportDirectory;
    j["exportQuickExportAsk"] = exportQuickExportAsk;

    std::ofstream fileStream(CONFIG_PATH);
    if (!fileStream.is_open()) {
        const std::string err = strerror(errno);
        throw Xcept("Failed to save file: {}, {}", CONFIG_PATH.string(), err);
    }
    fileStream << std::setw(2) << j << std::endl;
    fileStream.close();
}
