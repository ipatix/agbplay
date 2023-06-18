#include <iostream>
#include <cstring>
#include <cstdio>
#include <curses.h>
#include <portaudio.h>
#include <clocale>
#include <rtmidi/RtMidi.h>

#include "SoundData.h"
#include "Debug.h"
#include "WindowGUI.h"
#include "Xcept.h"
#include "ConfigManager.h"
#include "OS.h"

static void usage();
static void help();

int main(int argc, char *argv[]) 
{
    OS::CheckTerminal();

    if (!Debug::open(nullptr)) {
        std::cout << "Debug Init failed" << std::endl;
        return EXIT_FAILURE;
    }
    if (argc < 2 || argc > 4) {
        usage();
        return EXIT_FAILURE;
    }

    if (!strcmp("--help", argv[1])) {
        help();
        return EXIT_SUCCESS;
    }

    size_t songTablePos = UNKNOWN_TABLE;
    if (argc >= 3) {
        songTablePos = strtoul(argv[2], nullptr, 16);
    }

    int midiPortNumber = -1;
    if (argc >= 4) {
        midiPortNumber = atoi(argv[3]) - 1;
    }

    try {
        setlocale(LC_ALL, "");
        if (Pa_Initialize() != paNoError)
            throw Xcept("Couldn't init portaudio");
        std::cout << "Loading ROM..." << std::endl;

        Rom::CreateInstance(argv[1]);
        std::cout << "Loading Config..." << std::endl;
        ConfigManager::Instance().Load();
        ConfigManager::Instance().SetGameCode(Rom::Instance().GetROMCode());
        std::cout << "Reading Songtable" << std::endl;
        SongTable songTable(songTablePos);
        std::cout << "Initialization complete!" << std::endl;
        WindowGUI wgui(songTable, midiPortNumber);

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
    if (Pa_Terminate() != paNoError)
        std::cerr << "Error while terminating portaudio" << std::endl;
    Debug::close();
    return 0;
}

static void usage()
{
    std::cout << "Usage: ./agbplay <ROM.gba> [song table position (hex)] [midi "
                 "port number]"
              << std::endl;
}

static void listMidiInPorts()
{
    RtMidiIn midiin;
    unsigned int nPorts = midiin.getPortCount();
    std::cout << "\nThere are " << nPorts << " MIDI input sources available.\n";
    std::string portName;
    for (unsigned int i = 0; i < nPorts; i++) {
        try {
            portName = midiin.getPortName(i);
        } catch (RtMidiError &error) {
            continue;
        }
        std::cout << "  Input Port #" << i + 1 << ": " << portName << '\n';
    }
}

static void help()
{
    usage();
    listMidiInPorts();
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
