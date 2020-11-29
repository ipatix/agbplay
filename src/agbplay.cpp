#include <iostream>
#include <cstring>
#include <cstdio>
#include <curses.h>
#include <portaudio.h>
#include <clocale>

#include "Rom.h"
#include "SoundData.h"
#include "Debug.h"
#include "WindowGUI.h"
#include "Xcept.h"
#include "ConfigManager.h"
#include "OS.h"

using namespace std;
using namespace agbplay;

int main(int argc, char *argv[]) 
{
    OS::CheckTerminal();

    if (!open_debug(nullptr)) {
        cout << "Debug Init failed" << endl;
        return EXIT_FAILURE;
    }
    if (argc != 2) {
        cout << "Usage: ./agbplay <ROM.gba>" << endl;
        return EXIT_FAILURE;
    }
    if (!strcmp("--help", argv[1])) {
        cout << "Usage: ./agbplay <ROM.gba>" << endl << endl <<
            "Controls:" << endl <<
            "  - Arrow Keys or HJKL: Navigate through the program" << endl <<
            "  - Tab: Change between Playlist and Songlist" << endl <<
            "  - A: Add the selected song to the playlist" << endl <<
            "  - D: Delete the selected song from the playlist" << endl <<
            "  - T: Toggle whether the song should be output to a file (see R and E)" << endl <<
            "  - G: Drag the song through the playlist for ordering" << endl <<
            "  - I: Force Song Restart" << endl <<
            "  - O: Song Play/Pause" << endl <<
            "  - P: Force Song Stop" << endl <<
            "  - +=: Double the playback speed" << endl <<
            "  - -: Halve the playback speed" << endl <<
            "  - Enter: Toggle Track Muting" << endl <<
            "  - M: Mute selected Track" << endl <<
            "  - S: Solo selected Track" << endl <<
            "  - U: Unmute all Tracks" << endl <<
            "  - N: Rename the selected song in the playlisy" << endl <<
            "  - E: Export selected songs to individual track files (to \"workdirectory/wav\")" << endl <<
            "  - R: Export selected songs to files (non-split)" << endl <<
            "  - B: Benchmark, Run the export program but don't write to file" << endl <<
            "  - Q or Ctrl-D: Exit Program" << endl;
        return EXIT_SUCCESS;
    }
    try {
        setlocale(LC_ALL, "");
        if (Pa_Initialize() != paNoError)
            throw Xcept("Couldn't init portaudio");
        cout << "Loading ROM..." << endl;
        FileContainer fc(argv[1]);
        cout << "Loaded ROM successfully" << endl;
        Rom rom(fc);
        cout << "Created ROM object" << endl;
        ConfigManager::Instance().SetGameCode(rom.GetROMCode());
        cout << "Loaded Config" << endl;
        SoundData sdata(rom);
        cout << "Analyzed Sound Data" << endl;
        WindowGUI wgui(rom, sdata);

        chrono::nanoseconds frameTime(1000000000 / 60);

        auto lastTime = chrono::high_resolution_clock::now();

        while (wgui.Handle()) {
            auto newTime = chrono::high_resolution_clock::now();
            if (lastTime + frameTime > newTime) {
                this_thread::sleep_for(frameTime - (newTime - lastTime));
                lastTime = chrono::high_resolution_clock::now();
            } else {
                lastTime = newTime;
            }
        }
    } catch (const exception& e) {
        endwin();
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    }
    if (Pa_Terminate() != paNoError)
        cerr << "Error while terminating portaudio" << endl;
    close_debug();
    return 0;
}
