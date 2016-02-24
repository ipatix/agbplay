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
#include "MyException.h"

using namespace std;
using namespace agbplay;

int main(int argc, char *argv[]) {
    if (!__open_debug()) {
        cout << "Debug Init failed" << endl;
        return EXIT_FAILURE;
    }
    if (argc != 2) {
        cout << "Usage: ./agbplay <ROM.gba>" << endl;
        return EXIT_FAILURE;
    }
    try {
        setlocale(LC_ALL, "");
        if (Pa_Initialize() != paNoError)
            throw MyException("Couldn't init portaudio");
        cout << "Loading ROM..." << endl;
        FileContainer fc(argv[1]);
        cout << "Loaded ROM successfully" << endl;
        Rom rom(fc);
        cout << "Created ROM object" << endl;
        SoundData sdata(rom);
        cout << "Analyzed Sound Data" << endl;
        WindowGUI wgui(rom, sdata);

        // blocking call until program get's closed
        wgui.Handle();
    } catch (exception& e) {
        endwin();
        cerr << e.what() << endl;
        __print_debug(e.what());
        return EXIT_FAILURE;
    }
    if (Pa_Terminate() != paNoError)
        cerr << "Error while terminating portaudio" << endl;
    __close_debug();
    return 0;
}
