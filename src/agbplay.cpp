#include <iostream>
#include <cstring>
#include <cstdio>
#include <curses.h>
#include <portaudio.h>
#include <clocale>
#ifdef __APPLE__
    #include <libproc.h>
    #include <unistd.h>
    #include <sys/sysctl.h>
#endif

#include "Rom.h"
#include "SoundData.h"
#include "Debug.h"
#include "WindowGUI.h"
#include "Xcept.h"

using namespace std;
using namespace agbplay;


#ifdef __APPLE__

// http://www.objectpark.net/en/parentpid.html

#define OPProcessValueUnknown UINT_MAX

int OPParentIDForProcessID(int pid)
/*" Returns the parent process id 
     for the given process id (pid). "*/
{
    struct kinfo_proc info;
    size_t length = sizeof(struct kinfo_proc);
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, pid };
    if (sysctl(mib, 4, &info, &length, NULL, 0) < 0)
        return OPProcessValueUnknown;
    if (length == 0)
        return OPProcessValueUnknown;
    return info.kp_eproc.e_ppid;
}

// adapted from https://stackoverflow.com/a/8149198
/*
 * Attempts to check for Apple Terminal.
 * Apple Terminal performs hilariously bad in comparison
 * to iTerm2 to the point where it is almost unusable.
 */
int CheckForAppleTerminal() {
    pid_t pid = getpid();
    int ret;
    pid_t oldpid = getpid();
    char pathbuf[PROC_PIDPATHINFO_MAXSIZE];
    do  {
        ret = proc_pidpath (pid, pathbuf, sizeof(pathbuf));
        if (strncmp(pathbuf, "/Applications/Utilities/Terminal.app", 37) == 0) {
            return 1;
        }
        oldpid = pid;
        pid = (pid_t)OPParentIDForProcessID(pid);
    } while (ret > 0 && pid > 1 /* launchd */ && pid != oldpid && pid != OPProcessValueUnknown);
    return 0;
}
#endif

int main(int argc, char *argv[]) 
{
    #ifdef __APPLE__
    if (CheckForAppleTerminal() == 1) {
        cout << "It appears that you are using macOS's built-in Terminal.app." << endl << endl <<
                    "This terminal is prone to some serious lag with agbplay." << endl <<
                    "It is recommended to use iTerm2 (https://www.iterm2.com/), as it can" << endl <<
                    "run agbplay without any lag." << endl << endl << 
                    "Press Return to continue, or Ctrl+C to exit.";
        cin.ignore();
    }
    #endif
    if (!_open_debug()) {
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
    _close_debug();
    return 0;
}
