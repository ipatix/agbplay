#include "Settings.hpp"

#include "OS.hpp"
#include "Xcept.hpp"

#include <cerrno>
#include <fstream>
#include <nlohmann/json.hpp>

static const std::filesystem::path CONFIG_PATH = OS::GetLocalConfigDirectory() / "agbplay" / "config.json";
static const std::filesystem::path DEFAULT_EXPORT_DIRECTORY = OS::GetMusicDirectory() / "agbplay";
static const uint32_t DEFAULT_SAMPLERATE = 48000;
static const uint32_t DEFAULT_BIT_DEPTH = 32;
static const uint32_t DEFAULT_NUM_OUTPUT_BUFFERS = 1;
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
            playbackOutputNumBuffers = DEFAULT_NUM_OUTPUT_BUFFERS;
            exportSampleRate = DEFAULT_SAMPLERATE;
            exportBitDepth = DEFAULT_BIT_DEPTH;
            exportQuickExportDirectory = DEFAULT_EXPORT_DIRECTORY;
            exportQuickExportAsk = DEFAULT_QUICK_EXPORT_ASK;
            return;
        }
        const std::string err = strerror(errno);
        throw Xcept("Failed to open file: {}, {}", CONFIG_PATH.string(), err);
    }

    json j = json::parse(fileStream);
    fileStream.close();

    if (j.contains("playbackSampleRate") && j["playbackSampleRate"].is_number()) {
        playbackSampleRate = std::max<uint32_t>(1u, j["playbackSampleRate"]);
    } else {
        playbackSampleRate = DEFAULT_SAMPLERATE;
    }

    if (j.contains("playbackOutputNumBuffers") && j["playbackOutputNumBuffers"].is_number()) {
        playbackOutputNumBuffers = std::clamp<uint32_t>(j["playbackOutputNumBuffers"], 1u, 16u);
    } else {
        playbackOutputNumBuffers = DEFAULT_NUM_OUTPUT_BUFFERS;
    }

    if (j.contains("exportSampleRate") && j["exportSampleRate"].is_number()) {
        exportSampleRate = std::max<uint32_t>(1u, j["exportSampleRate"]);
    } else {
        exportSampleRate = DEFAULT_SAMPLERATE;
    }

    if (j.contains("exportBitDepth") && j["exportBitDepth"].is_number()) {
        exportBitDepth = std::max<uint32_t>(1u, j["exportBitDepth"]);
    } else {
        exportBitDepth = DEFAULT_BIT_DEPTH;
    }

    if (j.contains("exportPadStart") && j["exportPadStart"].is_number()) {
        exportPadStart = j["exportPadStart"];
    } else {
        exportPadStart = 0.0;
    }

    if (j.contains("exportPadEnd") && j["exportPadEnd"].is_number()) {
        exportPadEnd = j["exportPadEnd"];
    } else {
        exportPadEnd = 0.0;
    }

    if (j.contains("exportQuickExportDirectory") && j["exportQuickExportDirectory"].is_string()) {
        const std::string tmp = j["exportQuickExportDirectory"];
        exportQuickExportDirectory = std::u8string(reinterpret_cast<const char8_t *>(tmp.c_str()));
    } else {
        exportQuickExportDirectory = DEFAULT_EXPORT_DIRECTORY;
    }

    if (j.contains("exportQuickExportAsk") && j["exportQuickExportAsk"].is_boolean()) {
        exportQuickExportAsk = j["exportQuickExportAsk"];
    } else {
        exportQuickExportAsk = DEFAULT_QUICK_EXPORT_ASK;
    }

    if (j.contains("lastOpenFileDirectory") && j["lastOpenFileDirectory"].is_string()) {
        const std::string tmp = j["lastOpenFileDirectory"];
        lastOpenFileDirectory = std::u8string(reinterpret_cast<const char8_t *>(tmp.c_str()));
    } else {
        lastOpenFileDirectory = "";
    }
}

void Settings::Save()
{
    using nlohmann::json;

    json j;
    j["playbackSampleRate"] = playbackSampleRate;
    j["playbackOutputNumBuffers"] = playbackOutputNumBuffers;
    j["exportSampleRate"] = exportSampleRate;
    j["exportBitDepth"] = exportBitDepth;
    j["exportPadStart"] = exportPadStart;
    j["exportPadEnd"] = exportPadEnd;
    j["exportQuickExportDirectory"] = exportQuickExportDirectory;
    j["exportQuickExportAsk"] = exportQuickExportAsk;
    j["lastOpenFileDirectory"] = lastOpenFileDirectory;

    std::ofstream fileStream(CONFIG_PATH);
    if (!fileStream.is_open()) {
        const std::string err = strerror(errno);
        throw Xcept("Failed to save file: {}, {}", CONFIG_PATH.string(), err);
    }
    fileStream << std::setw(2) << j << std::endl;
    fileStream.close();
}
