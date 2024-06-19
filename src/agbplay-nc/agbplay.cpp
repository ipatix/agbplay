#include <iostream>
#include <cstring>
#include <cstdio>
#include <curses.h>
#include <portaudiocpp/AutoSystem.hxx>
#include <clocale>
#include <fmt/core.h>

#include "Debug.h"
#include "WindowGUI.h"
#include "Xcept.h"
#include "ConfigManager.h"
#include "OS.h"
#include "MP2KScanner.h"
#include "ProfileManager.h"

static void usage();
static void help();

int main(int argc, char *argv[])
{
    OS::CheckTerminal();

    if (!Debug::open(nullptr)) {
        std::cout << "Debug Init failed" << std::endl;
        return EXIT_FAILURE;
    }
    if (argc != 2 && argc != 3) {
        usage();
        return EXIT_FAILURE;
    }
    if (!strcmp("--help", argv[1])) {
        help();
        return EXIT_SUCCESS;
    }
    unsigned long songTableIndex = 0;
    if (argc == 3) {
      try {
        songTableIndex = std::stoul(argv[2]);
      } catch (std::exception& e) {
        usage();
        return EXIT_FAILURE;
      }
    }
    try {
        ConfigManager &cfm = ConfigManager::Instance();
        setlocale(LC_ALL, "");

        portaudio::AutoSystem paSystem;
        fmt::print("Loading ROM...\n");
        Rom::CreateInstance(argv[1]);

        fmt::print("Loading Config...\n");
        cfm.Load();

        fmt::print("Loading Profiles...\n");
        ProfileManager pm;
        pm.SetPath(OS::GetLocalConfigDirectory() / "agbplay" / "profiles");
        pm.LoadProfiles();
        pm.SaveProfiles();

        fmt::print("Scanning for MP2K Engine\n");
        MP2KScanner scanner(Rom::Instance());
        auto scanResults = scanner.Scan();
        fmt::print(" -> Found {} instance(s)\n", scanResults.size());

        for (size_t i = 0; i < scanResults.size(); i++) {
            auto &r = scanResults.at(i);
            fmt::print("  - songtable: 0x{:x}\n", r.songTableInfo.pos);
            fmt::print("    song count: {}\n", r.songTableInfo.count);
            fmt::print("    sound mode:\n");
            fmt::print("      pcm vol: {}\n", r.mp2kSoundMode.vol);
            fmt::print("      pcm rev: {}\n", r.mp2kSoundMode.rev);
            fmt::print("      pcm freq: {}\n", r.mp2kSoundMode.freq);
            fmt::print("      pcm channels: {}\n", r.mp2kSoundMode.maxChannels);
            fmt::print("      dac config: {}\n", r.mp2kSoundMode.dacConfig);
            fmt::print("    player table (count={}):\n", r.playerTableInfo.size());
            for (auto &i : r.playerTableInfo)
                fmt::print("      max track={}, use prio={}\n", i.maxTracks, i.usePriority);
        }

        if (songTableIndex >= scanResults.size()) {
            if (songTableIndex == 0)
                throw Xcept("Unable to find Songtable");
            else
                throw Xcept("Songtable index out of range");
        }

        std::string gameCode = Rom::Instance().GetROMCode();
        if (songTableIndex > 0)
            gameCode = fmt::format("{}:{}", gameCode, songTableIndex);
        cfm.SetGameCode(gameCode);
        auto scanResult = scanResults.at(songTableIndex);

        fmt::print("Initialization complete!\n");
        WindowGUI wgui(scanResult.songTableInfo, scanResult.playerTableInfo);

        std::chrono::nanoseconds frameTime(1000000000 / 60);

        auto lastTime = std::chrono::high_resolution_clock::now();

        while (wgui.Handle()) {
            auto newTime = std::chrono::high_resolution_clock::now();
            if (lastTime + frameTime > newTime) {
                std::this_thread::sleep_for(frameTime - (newTime - lastTime));
                lastTime = std::chrono::high_resolution_clock::now();
            } else {
                lastTime = newTime;
            }
        }
    } catch (const std::exception& e) {
        endwin();
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    Debug::close();
    return 0;
}

static void usage() {
    std::cout << "Usage: ./agbplay <ROM.gba> [table number]" << std::endl;
}

static void help() {
    usage();
    std::cout << "\nControls:\n"
        "  - Arrow Keys or HJKL: Navigate through the program\n"
        "  - Tab: Change between Playlist and Songlist\n"
        "  - A: Add the selected song to the playlist\n"
        "  - D: Delete the selected song from the playlist\n"
        "  - T: Toggle whether the song should be output to a file (see R and E)\n"
        "  - G: Drag the song through the playlist for ordering\n"
        "  - I: Force Song Restart\n"
        "  - O: Song Play/Pause\n"
        "  - P: Force Song Stop\n"
        "  - +=: Double the playback speed\n"
        "  - -: Halve the playback speed\n"
        "  - Enter: Toggle Track Muting\n"
        "  - M: Mute selected Track\n"
        "  - S: Solo selected Track\n"
        "  - U: Unmute all Tracks\n"
        "  - N: Rename the selected song in the playlisy\n"
        "  - E: Export selected songs to individual track files (to \"workdirectory/wav\")\n"
        "  - R: Export selected songs to files (non-split)\n"
        "  - B: Benchmark, Run the export program but don't write to file\n"
        "  - Q or Ctrl-D: Exit Program\n" << std::flush;
}
