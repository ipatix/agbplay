#include "CLIProfile.hpp"

#include "ProfileManager.hpp"
#include "Rom.hpp"

#include <algorithm>
#include <fmt/core.h>
#include <ranges>
#include <sstream>

static std::string indent(const std::string& input, size_t indent) {
    std::string indentStr(indent, ' ');
    std::istringstream iss(input);
    std::string result;
    std::string line;

    while (std::getline(iss, line)) {
        if (std::all_of(line.begin(), line.end(), [](char c) { return std::isspace(c) != 0; }))
            continue;
        result += indentStr + line + "\n";
    }

    return result;
}

void CLI::ProfileShow()
{
    ProfileManager pm;
    pm.LoadProfiles();

    const auto p = pm.GetCLIDefaultProfile(Rom::Instance());

    fmt::print("Selected Profile '{}':\n", p->path.string());
    fmt::print("  Name:        {}\n", p->name);
    fmt::print("  Author:      {}\n", p->author);
    fmt::print("  Game Studio: {}\n", p->gameStudio);
    fmt::print("  Description:\n{}", indent(p->description, 4));
    fmt::print("  Notes:\n{}", indent(p->description, 4));

    fmt::print("  Game Match:\n");
    fmt::print("    Game Codes: [{}]\n", p->gameMatch.gameCodes | std::views::join_with(std::string_view(", ")) | std::ranges::to<std::string>());
    if (p->gameMatch.magicBytes.size() > 0) {
        fmt::print("    Magic Bytes: [{}]\n",
            p->gameMatch.magicBytes |
            std::views::transform([](uint8_t b) { return fmt::format("{:02x}", b); }) |
            std::views::join_with(std::string_view(", ")) |
            std::ranges::to<std::string>()
        );
    }

    fmt::print("  Song Table:\n");

    if (p->songTableInfoConfig.IsAuto()) {
        if (p->songTableInfoScanned.IsAuto()) {
            fmt::print("    Offset:    automatic\n");
            fmt::print("    Num Songs: automatic\n");
            fmt::print("    Index:     {}\n", p->songTableInfoConfig.tableIdx);
        } else {
            fmt::print("    Offset:    automatic ({:#x})\n", p->songTableInfoScanned.pos);
            fmt::print("    Num Songs: automatic ({})\n", p->songTableInfoScanned.count);
            fmt::print("    Index:     {}\n", p->songTableInfoConfig.tableIdx);
        }
    } else {
        fmt::print("    Offset:    {:#x}\n", p->songTableInfoConfig.pos);
        fmt::print("    Num Songs: {}\n", p->songTableInfoConfig.count);
    }

    if (p->playerTableConfig.size() == 0) {
        if (p->playerTableScanned.size() == 0) {
            fmt::print("  Player Table: auto\n");
        } else {
            fmt::print("  Player Table: auto (count={})\n", p->playerTableScanned.size());
            for (size_t i = 0; i < p->playerTableScanned.size(); i++) {
                fmt::print("    - [{:02d}] maxTracks: {: >2d}, usePriority: {}\n",
                    i,
                    p->playerTableScanned.at(i).maxTracks,
                    static_cast<bool>(p->playerTableScanned.at(i).usePriority)
                );
            }
        }
    } else {
        fmt::print("  Player Table: manual (count={})\n", p->playerTableConfig.size());
        for (size_t i = 0; i < p->playerTableConfig.size(); i++) {
            fmt::print("    - [{:02d}] maxTracks: {: >2d}, usePriority: {}\n",
                i,
                p->playerTableConfig.at(i).maxTracks,
                static_cast<bool>(p->playerTableConfig.at(i).usePriority)
            );
        }
    }

    if (p->mp2kSoundModeConfig.IsAuto()) {
        if (!p->mp2kSoundModeScanned.IsAuto()) {
            fmt::print("  MP2K Sound Mode: automatic (scanned)\n");
            fmt::print("    Volume:     {} / 15\n", p->mp2kSoundModeScanned.vol);
            fmt::print("    Reverb:     {:#04x}\n", p->mp2kSoundModeScanned.rev);
            fmt::print("    Frequency:  {}\n", p->mp2kSoundModeScanned.freq);
            fmt::print("    Max Chn:    {} / 12\n", p->mp2kSoundModeScanned.maxChannels);
            fmt::print("    DAC Config: {}\n", p->mp2kSoundModeScanned.dacConfig);
        } else {
            fmt::print("  MP2K Sound Mode: automatic (not scanned)\n");
        }
    } else {
        fmt::print("  MP2K Sound Mode: manual\n");
        fmt::print("    Volume:     {} / 15\n", p->mp2kSoundModeConfig.vol);
        fmt::print("    Reverb:     {:#04x}\n", p->mp2kSoundModeConfig.rev);
        fmt::print("    Frequency:  {}\n", p->mp2kSoundModeConfig.freq);
        fmt::print("    Max Chn:    {} / 12\n", p->mp2kSoundModeConfig.maxChannels);
        fmt::print("    DAC Config: {}\n", p->mp2kSoundModeConfig.dacConfig);
    }

    fmt::print("  agbplay Sound Mode:\n");
    fmt::print("    Resampler (normal): {}\n", res2str(p->agbplaySoundMode.resamplerTypeNormal));
    fmt::print("    Resampler (fixed): {}\n", res2str(p->agbplaySoundMode.resamplerTypeFixed));
    fmt::print("    Reverb Type: {}\n", rev2str(p->agbplaySoundMode.reverbType));
    fmt::print("    PSG Polyphony: {}\n", cgbPoly2str(p->agbplaySoundMode.cgbPolyphony));
    fmt::print("    DMA Buffer Len: {:#x}\n", p->agbplaySoundMode.dmaBufferLen);
    fmt::print("    Accurate CH3 Quant: {}\n", p->agbplaySoundMode.accurateCh3Quantization);
    fmt::print("    Accurate CH3 Vol: {}\n", p->agbplaySoundMode.accurateCh3Volume);
    fmt::print("    Emulate PSG Sustain Bug: {}\n", p->agbplaySoundMode.emulateCgbSustainBug);
}

void CLI::ProfileList()
{
    ProfileManager pm;
    pm.LoadProfiles();

    auto profiles = pm.GetAllProfiles();

    fmt::print("    Game Codes    |         Name         |           Path\n");
    fmt::print("------------------+----------------------+--------------------------\n");

    for (const auto &p : profiles) {
        std::string codes = p->gameMatch.gameCodes | std::views::join_with(',') | std::ranges::to<std::string>();
        if (codes.size() > 14)
            codes = codes.substr(0, 14) + "..";
        std::string name = p->name;
        if (name.size() > 18)
            name = name.substr(0, 18) + "..";
        fmt::print(" {: ^16s} | {:20s} | {}\n", codes, name, p->path.string());
    }
}
