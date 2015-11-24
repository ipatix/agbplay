#include <iostream>
#include <cstring>
#include <cstdio>
#include <ncurses.h>
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
        cout << "Loading ROM..." << endl;
        Rom rom(argv[1]);
        SoundData sdata(rom);
        WindowGUI wgui(rom, sdata);
        wgui.Handle();
    } catch (const exception& e) {
        endwin();
        cout << e.what() << endl;
        return EXIT_FAILURE;
    }
    __close_debug();
    return 0;
}
