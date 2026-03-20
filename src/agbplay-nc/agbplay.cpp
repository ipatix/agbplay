#include "Debug.hpp"
#include "MP2KScanner.hpp"
#include "OS.hpp"
#include "ProfileManager.hpp"
#include "Settings.hpp"
#include "WindowGUI.hpp"
#include "Xcept.hpp"

#include <clocale>
#include <cstdio>
#include <cstring>
#include <curses.h>
#include <fmt/core.h>
#include <iostream>
#include <portaudiocpp/AutoSystem.hxx>

static void usage();
static void help();

int main(int argc, char *argv[])
{
    OS::CheckTerminal();

    if (!Debug::open(nullptr)) {
        std::cout << "Debug Init failed" << std::endl;
        return EXIT_FAILURE;
    }
    if (argc != 2) {
        usage();
        return EXIT_FAILURE;
    }
    if (!strcmp("--help", argv[1])) {
        help();
        return EXIT_SUCCESS;
    }
    try {
        setlocale(LC_ALL, "");

        portaudio::AutoSystem paSystem;
        fmt::print("Loading ROM...\n");
        Rom::CreateInstance(argv[1]);

        fmt::print("Loading Profiles...\n");
        ProfileManager pm;
        pm.LoadProfiles();

        fmt::print("Opening profile...\n");

        fmt::print("Creating GUI!\n");
        WindowGUI wgui(*pm.GetCLIDefaultProfile(Rom::Instance()));

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

        pm.SaveProfiles();
    } catch (const std::exception &e) {
        echo();
        endwin();
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    Debug::close();
    return 0;
}

static void usage()
{
    std::cout << "Usage: ./agbplay <ROM.gba> [table number]" << std::endl;
}

static void help()
{
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
                 "  - Q or Ctrl-D: Exit Program\n"
              << std::flush;
}
