#include <ncurses.h>
#include <string>
#include <iomanip>
#include <cstdlib>
#include <stdexcept>

#include "MyException.h"
#include "Debug.h"
#include "ColorDef.h"
#include "WindowGUI.h"
#include "Util.h"

#define KEY_TAB 9

using namespace agbplay;
using namespace std;

WindowGUI::WindowGUI(Rom& rrom, SoundData& rsdata) : rom(rrom), sdata(rsdata) {
    // init ncurses stuff
    CursesWin::UIMutex.lock();
    containerWin = initscr();
    getmaxyx(stdscr, height, width);
    if (has_colors() == false)
        throw MyException("Error, your terminal doesn't support colors");
    CursesWin::UIMutex.unlock();
    initColors();
    CursesWin::UIMutex.lock();
    noecho();
    curs_set(0);
    keypad(stdscr, true);
    CursesWin::UIMutex.unlock();

    cursorl = SONGLIST;

    // cap small terminal sizes
    if (height < 24) height = 24;
    if (width < 80) width = 80;

    // create subwindows
    conUI = new ConsoleGUI(
            CONSOLE_HEIGHT(height, width),
            CONSOLE_WIDTH(height, width),
            CONSOLE_YPOS(height, width),
            CONSOLE_XPOS(height, width));

    hotUI = new HotkeybarGUI(
            HOTKEYBAR_HEIGHT(height, width),
            HOTKEYBAR_WIDTH(height, width),
            HOTKEYBAR_YPOS(height, width),
            HOTKEYBAR_XPOS(height, width));

    songUI = new SonglistGUI(
            SONGLIST_HEIGHT(height, width),
            SONGLIST_WIDTH(height, width),
            SONGLIST_YPOS(height, width),
            SONGLIST_XPOS(height, width), true);

    // add songs to table
    for (uint16_t i = 0; i < sdata.sTable->GetNumSongs(); i++) {
        ostringstream txt;
        txt << setw(4) << setfill('0') << i;
        songUI->AddSong(SongEntry(txt.str(), i));
    }
    songUI->Enter();

    playUI = new PlaylistGUI(
            PLAYLIST_HEIGHT(height, width),
            PLAYLIST_WIDTH(height, width),
            PLAYLIST_YPOS(height, width),
            PLAYLIST_XPOS(height, width));

    titleUI = new TitlebarGUI(
            TITLEBAR_HEIGHT(height, width),
            TITLEBAR_WIDTH(height, width),
            TITLEBAR_YPOS(height, width),
            TITLEBAR_XPOS(height, width));

    romUI = new RomviewGUI(
            ROMVIEW_HEIGHT(height, width),
            ROMVIEW_WIDTH(height, width),
            ROMVIEW_YPOS(height, width),
            ROMVIEW_XPOS(height, width),
            rom, sdata);

    trackUI = new TrackviewGUI(
            TRACKVIEW_HEIGHT(height, width),
            TRACKVIEW_WIDTH(height, width),
            TRACKVIEW_YPOS(height, width),
            TRACKVIEW_XPOS(height, width));
    
    rom.Seek(sdata.sTable->GetSongTablePos());
    mplay = new PlayerModule(rom, trackUI, rom.ReadAGBPtrToPos());
    mplay->LoadSong(sdata.sTable->GetPosOfSong(0));
}

WindowGUI::~WindowGUI() {
    delete conUI;
    delete hotUI;
    delete songUI;
    delete playUI;
    delete titleUI;
    delete romUI;
    delete trackUI;
    delete mplay;
    CursesWin::UIMutex.lock();
    endwin();
    CursesWin::UIMutex.unlock();
}

void WindowGUI::Handle() {
    while (true) {
        int ch = conUI->ConGetCH();
        switch (ch) {
            case KEY_RESIZE:
                getmaxyx(stdscr, height, width);
                if (height < 24) height = 24;
                if (width < 80) width = 80;
                resizeWindows();
                break;
            case KEY_UP:
                scrollUp();
                break;
            case KEY_DOWN:
                scrollDown();
                break;
            case KEY_PPAGE:
                pageUp();
                break;
            case KEY_NPAGE:
                pageDown();
                break;
            case KEY_TAB:
                cycleFocus();
                break;
            case 'a':
                add();
                break;
            case 'd':
                del();
                break;
            case 't':
                if (cursorl == PLAYLIST)
                    playUI->ToggleTick();
                break;
            case 'g':
                if (cursorl == PLAYLIST)
                    playUI->ToggleDrag();
                break;
            case EOF:
            case 4: // EOT
            case 'q':
                conUI->WriteLn("Exiting...");
                return;
            default:
                string msg = "Bad, you pressed ";
                msg += to_string(ch);
                conUI->WriteLn(msg.c_str());
                __print_debug("msg");
        }
    }
}

void WindowGUI::resizeWindows() {
    // TODO fill in all windows
    __print_debug("Resizing Window...");
    conUI->Resize(
            CONSOLE_HEIGHT(height, width),
            CONSOLE_WIDTH(height, width),
            CONSOLE_YPOS(height, width),
            CONSOLE_XPOS(height, width));
    hotUI->Resize(
            HOTKEYBAR_HEIGHT(height, width),
            HOTKEYBAR_WIDTH(height, width),
            HOTKEYBAR_YPOS(height, width),
            HOTKEYBAR_XPOS(height, width));
    songUI->Resize(
            SONGLIST_HEIGHT(height, width),
            SONGLIST_WIDTH(height, width),
            SONGLIST_YPOS(height, width),
            SONGLIST_XPOS(height, width));
    playUI->Resize(
            PLAYLIST_HEIGHT(height, width),
            PLAYLIST_WIDTH(height, width),
            PLAYLIST_YPOS(height, width),
            PLAYLIST_XPOS(height, width));
    titleUI->Resize(
            TITLEBAR_HEIGHT(height, width),
            TITLEBAR_WIDTH(height, width),
            TITLEBAR_YPOS(height, width),
            TITLEBAR_XPOS(height, width));
    romUI->Resize(
            ROMVIEW_HEIGHT(height, width),
            ROMVIEW_WIDTH(height, width),
            ROMVIEW_YPOS(height, width),
            ROMVIEW_XPOS(height, width));
    trackUI->Resize(
            TRACKVIEW_HEIGHT(height, width),
            TRACKVIEW_WIDTH(height, width),
            TRACKVIEW_YPOS(height, width),
            TRACKVIEW_XPOS(height, width));
}

void WindowGUI::initColors() {
    CursesWin::UIMutex.lock();
    start_color();
    if (use_default_colors() == ERR)
        throw MyException("Using default terminal colors failed");
    init_pair(DEFAULT_ON_DEFAULT, -1, -1);
    init_pair(RED_ON_DEFAULT, COLOR_RED, -1);
    init_pair(GREEN_ON_DEFAULT, COLOR_GREEN, -1);
    init_pair(YELLOW_ON_DEFAULT, COLOR_YELLOW, -1);
    init_pair(CYAN_ON_DEFAULT, COLOR_CYAN, -1);
    CursesWin::UIMutex.unlock();
}

void WindowGUI::cycleFocus() {
    switch (cursorl) {
        case SONGLIST:
            songUI->Leave();
            cursorl = PLAYLIST;
            playUI->Enter();
            TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(playUI->GetSong().GetUID())));
            break;
        case PLAYLIST:
            playUI->Leave();
            cursorl = SONGLIST;
            songUI->Enter();
            TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(songUI->GetSong().GetUID())));
            break;
    }
}

void WindowGUI::scrollDown() {
    switch (cursorl) {
        case SONGLIST:
            songUI->ScrollDown();
            TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(songUI->GetSong().GetUID())));
            break;
        case PLAYLIST:
            playUI->ScrollDown();
            TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(playUI->GetSong().GetUID())));
            break;
    }
}

void WindowGUI::scrollUp() {
    switch (cursorl) {
        case SONGLIST:
            songUI->ScrollUp();
            TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(songUI->GetSong().GetUID())));
            break;
        case PLAYLIST:
            playUI->ScrollUp();
            TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(playUI->GetSong().GetUID())));
            break;
    }
}

void WindowGUI::pageDown() {
    switch (cursorl) {
        case SONGLIST:
            songUI->PageDown();
            TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(songUI->GetSong().GetUID())));
            break;
        case PLAYLIST:
            playUI->PageDown();
            TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(playUI->GetSong().GetUID())));
            break;
    }
}

void WindowGUI::pageUp() {
    switch (cursorl) {
        case SONGLIST:
            songUI->PageUp();
            TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(songUI->GetSong().GetUID())));
            break;
        case PLAYLIST:
            playUI->PageUp();
            TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(playUI->GetSong().GetUID())));
            break;
    }
}

void WindowGUI::add() {
    if (cursorl != SONGLIST) return;
    TRY_OOR(playUI->AddSong(songUI->GetSong()));

}

void WindowGUI::del() {
    if (cursorl != PLAYLIST) return;
    playUI->RemoveSong();
    TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(playUI->GetSong().GetUID())));
}
