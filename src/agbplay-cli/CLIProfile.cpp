#include "CLIProfile.hpp"

#include "ProfileManager.hpp"
#include "Rom.hpp"

#include <algorithm>
#include <print>
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

    std::println("Selected Profile '{}':", p->path.string());
    std::println("  Name:        {}", p->name);
    std::println("  Author:      {}", p->author);
    std::println("  Game Studio: {}", p->gameStudio);
    std::print("  Description:\n{}", indent(p->description, 4));
    std::println("  Notes:{}", indent(p->description, 4));

    std::println("  Game Match:");
    std::println("    Game Codes: [{}]", p->gameMatch.gameCodes | std::views::join_with(std::string_view(", ")) | std::ranges::to<std::string>());
    if (p->gameMatch.magicBytes.size() > 0) {
        std::println("    Magic Bytes: [{}]",
            p->gameMatch.magicBytes |
            std::views::transform([](uint8_t b) { return std::format("{:02x}", b); }) |
            std::views::join_with(std::string_view(", ")) |
            std::ranges::to<std::string>()
        );
    }

    std::println("  Song Table:");

    if (p->songTableInfoConfig.IsAuto()) {
        if (p->songTableInfoScanned.IsAuto()) {
            std::println("    Offset:    automatic");
            std::println("    Num Songs: automatic");
            std::println("    Index:     {}", p->songTableInfoConfig.tableIdx);
        } else {
            std::println("    Offset:    automatic ({:#x})", p->songTableInfoScanned.pos);
            std::println("    Num Songs: automatic ({})", p->songTableInfoScanned.count);
            std::println("    Index:     {}", p->songTableInfoConfig.tableIdx);
        }
    } else {
        std::println("    Offset:    {:#x}", p->songTableInfoConfig.pos);
        std::println("    Num Songs: {}", p->songTableInfoConfig.count);
    }

    if (p->playerTableConfig.size() == 0) {
        if (p->playerTableScanned.size() == 0) {
            std::println("  Player Table: auto");
        } else {
            std::println("  Player Table: auto (count={})", p->playerTableScanned.size());
            for (size_t i = 0; i < p->playerTableScanned.size(); i++) {
                std::println("    - [{:02d}] maxTracks: {: >2d}, usePriority: {}",
                    i,
                    p->playerTableScanned.at(i).maxTracks,
                    static_cast<bool>(p->playerTableScanned.at(i).usePriority)
                );
            }
        }
    } else {
        std::println("  Player Table: manual (count={})", p->playerTableConfig.size());
        for (size_t i = 0; i < p->playerTableConfig.size(); i++) {
            std::println("    - [{:02d}] maxTracks: {: >2d}, usePriority: {}",
                i,
                p->playerTableConfig.at(i).maxTracks,
                static_cast<bool>(p->playerTableConfig.at(i).usePriority)
            );
        }
    }

    if (p->mp2kSoundModeConfig.IsAuto()) {
        if (!p->mp2kSoundModeScanned.IsAuto()) {
            std::println("  MP2K Sound Mode: automatic (scanned)");
            std::println("    Volume:     {} / 15", p->mp2kSoundModeScanned.vol);
            std::println("    Reverb:     {:#04x}", p->mp2kSoundModeScanned.rev);
            std::println("    Frequency:  {}", p->mp2kSoundModeScanned.freq);
            std::println("    Max Chn:    {} / 12", p->mp2kSoundModeScanned.maxChannels);
            std::println("    DAC Config: {}", p->mp2kSoundModeScanned.dacConfig);
        } else {
            std::println("  MP2K Sound Mode: automatic (not scanned)");
        }
    } else {
        std::println("  MP2K Sound Mode: manual");
        std::println("    Volume:     {} / 15", p->mp2kSoundModeConfig.vol);
        std::println("    Reverb:     {:#04x}", p->mp2kSoundModeConfig.rev);
        std::println("    Frequency:  {}", p->mp2kSoundModeConfig.freq);
        std::println("    Max Chn:    {} / 12", p->mp2kSoundModeConfig.maxChannels);
        std::println("    DAC Config: {}", p->mp2kSoundModeConfig.dacConfig);
    }

    std::println("  agbplay Sound Mode:");
    std::println("    Resampler (normal): {}", res2str(p->agbplaySoundMode.resamplerTypeNormal));
    std::println("    Resampler (fixed): {}", res2str(p->agbplaySoundMode.resamplerTypeFixed));
    std::println("    Reverb Type: {}", rev2str(p->agbplaySoundMode.reverbType));
    std::println("    PSG Polyphony: {}", cgbPoly2str(p->agbplaySoundMode.cgbPolyphony));
    std::println("    DMA Buffer Len: {:#x}", p->agbplaySoundMode.dmaBufferLen);
    std::println("    Accurate CH3 Quant: {}", p->agbplaySoundMode.accurateCh3Quantization);
    std::println("    Accurate CH3 Vol: {}", p->agbplaySoundMode.accurateCh3Volume);
    std::println("    Emulate PSG Sustain Bug: {}", p->agbplaySoundMode.emulateCgbSustainBug);
}

void CLI::ProfileList()
{
    ProfileManager pm;
    pm.LoadProfiles();

    auto profiles = pm.GetAllProfiles();

    std::println("    Game Codes    |         Name         |           Path");
    std::println("------------------+----------------------+--------------------------");

    for (const auto &p : profiles) {
        std::string codes = p->gameMatch.gameCodes | std::views::join_with(',') | std::ranges::to<std::string>();
        if (codes.size() > 14)
            codes = codes.substr(0, 14) + "..";
        std::string name = p->name;
        if (name.size() > 18)
            name = name.substr(0, 18) + "..";
        std::println(" {: ^16s} | {:20s} | {}", codes, name, p->path.string());
    }
}
