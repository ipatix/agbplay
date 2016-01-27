#include <string>
#include <cstdlib>
#include <stdexcept>
#include <thread>
#include <chrono>

#include "MyException.h"
#include "Debug.h"
#include "ColorDef.h"
#include "WindowGUI.h"
#include "Util.h"

#define KEY_TAB 9

using namespace agbplay;
using namespace std;

WindowGUI::WindowGUI(Rom& rrom, SoundData& rsdata) : rom(rrom), sdata(rsdata) 
{
    // init ncurses stuff
    //CursesWin::UIMutex.lock();
    this->containerWin = initscr();
    getmaxyx(stdscr, height, width);
    if (has_colors() == false)
        throw MyException("Error, your terminal doesn't support colors");
    //CursesWin::UIMutex.unlock();
    initColors();
    //CursesWin::UIMutex.lock();
    noecho();
    curs_set(0);
    keypad(stdscr, true);
    //CursesWin::UIMutex.unlock();

    this->play = false;
    this->cursorl = SONGLIST;

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

    rom.Seek(0xAC);
    string gameCode = rom.ReadString(4);
    playUI = new PlaylistGUI(
            PLAYLIST_HEIGHT(height, width),
            PLAYLIST_WIDTH(height, width),
            PLAYLIST_YPOS(height, width),
            PLAYLIST_XPOS(height, width),
            gameCode);

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

    meterUI = new VUMeterGUI(
            VUMETER_HEIGHT(height, width),
            VUMETER_WIDTH(height, width),
            VUMETER_YPOS(height, width),
            VUMETER_XPOS(height, width));

    rom.Seek(sdata.sTable->GetSongTablePos());
    mplay = new PlayerInterface(rom, trackUI, rom.ReadAGBPtrToPos(), EnginePars(15, 0, 4));
    mplay->LoadSong(sdata.sTable->GetPosOfSong(0), 16); // TODO read track limit from rom rather than using fixed value
}

WindowGUI::~WindowGUI() 
{
    delete mplay;
    delete conUI;
    delete hotUI;
    delete songUI;
    delete playUI;
    delete titleUI;
    delete romUI;
    delete trackUI;
    delete meterUI;
    endwin();
}

void WindowGUI::Handle() 
{
    while (true) {
        int ch;
        while ((ch = titleUI->GetKey()) != ERR) {
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
                case KEY_LEFT:
                    scrollLeft();
                    break;
                case KEY_RIGHT:
                    scrollRight();
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
                case 'i':
                    mplay->Play();
                    play = true;
                    break;
                case 'o':
                case ' ':
                    mplay->Pause();
                    break;
                case 'p':
                    play = false;
                    mplay->Stop();
                    break;
                case '+':
                    mplay->SpeedDouble();
                    break;
                case '-':
                    mplay->SpeedHalve();
                    break;
                case EOF:
                case 4: // EOT
                case 'q':
                    conUI->WriteLn("Exiting...");
                    mplay->Stop();
                    return;
            } // end key handling switch
        } // end key loop
        mplay->UpdateView();
        if (!mplay->IsPlaying() && play) {
            if (cursorl != PLAYLIST && cursorl != SONGLIST) {
                play = false;
            } else {
                scrollDown();
                mplay->Play();
            }
        }
        float lVol;
        float rVol;
        mplay->GetVolLevels(lVol, rVol);
        meterUI->SetVol(lVol, rVol);
        this_thread::sleep_for(chrono::milliseconds(25));
    } // end rendering loop
}

void WindowGUI::resizeWindows() 
{
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
    meterUI->Resize(
            VUMETER_HEIGHT(height, width), 
            VUMETER_WIDTH(height, width),
            VUMETER_YPOS(height, width),
            VUMETER_XPOS(height, width));
}

void WindowGUI::initColors() 
{
    //CursesWin::UIMutex.lock();
    start_color();
    if (use_default_colors() == ERR)
        throw MyException("Using default terminal colors failed");
    init_pair((int)Color::DEF_DEF, -1, -1);
    init_pair((int)Color::RED_DEF, COLOR_RED, -1);
    init_pair((int)Color::GRN_DEF, COLOR_GREEN, -1);
    init_pair((int)Color::YEL_DEF, COLOR_YELLOW, -1);
    init_pair((int)Color::CYN_DEF, COLOR_CYAN, -1);
    init_pair((int)Color::MAG_DEF, COLOR_MAGENTA, -1);
    //CursesWin::UIMutex.unlock();
}

void WindowGUI::cycleFocus() 
{
    switch (cursorl) {
        case SONGLIST:
            songUI->Leave();
            cursorl = PLAYLIST;
            playUI->Enter();
            TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(playUI->GetSong().GetUID()), 16));
            break;
        case PLAYLIST:
            playUI->Leave();
            cursorl = SONGLIST;
            songUI->Enter();
            TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(songUI->GetSong().GetUID()), 16));
            break;
        default:
            break;
    }
}

void WindowGUI::scrollLeft()
{
    switch (cursorl) {
        case TRACKS:
            trackUI->Leave();
            cursorl = SONGLIST;
            songUI->Enter();
            break;
        default:
            break;
    }
}

void WindowGUI::scrollRight()
{
    switch (cursorl) {
        case SONGLIST:
            songUI->Leave();
            cursorl = TRACKS;
            trackUI->Enter();
            break;
        case PLAYLIST:
            playUI->Leave();
            cursorl = TRACKS;
            trackUI->Enter();
            break;
        default:
            break;
    }
}

void WindowGUI::scrollDown() 
{
    switch (cursorl) {
        case SONGLIST:
            songUI->ScrollDown();
            TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(songUI->GetSong().GetUID()), 16));
            break;
        case PLAYLIST:
            playUI->ScrollDown();
            TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(playUI->GetSong().GetUID()), 16));
            break;
        case TRACKS:
            trackUI->ScrollDown();
            break;
        default:
            break;
    }
}

void WindowGUI::scrollUp() 
{
    switch (cursorl) {
        case SONGLIST:
            songUI->ScrollUp();
            TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(songUI->GetSong().GetUID()), 16));
            break;
        case PLAYLIST:
            playUI->ScrollUp();
            TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(playUI->GetSong().GetUID()), 16));
            break;
        case TRACKS:
            trackUI->ScrollUp();
            break;
        default:
            break;
    }
}

void WindowGUI::pageDown() 
{
    switch (cursorl) {
        case SONGLIST:
            songUI->PageDown();
            TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(songUI->GetSong().GetUID()), 16));
            break;
        case PLAYLIST:
            playUI->PageDown();
            TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(playUI->GetSong().GetUID()), 16));
            break;
        case TRACKS:
            trackUI->PageDown();
            break;
        default:
            break;
    }
}

void WindowGUI::pageUp() 
{
    switch (cursorl) {
        case SONGLIST:
            songUI->PageUp();
            TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(songUI->GetSong().GetUID()), 16));
            break;
        case PLAYLIST:
            playUI->PageUp();
            TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(playUI->GetSong().GetUID()), 16));
            break;
        case TRACKS:
            trackUI->PageUp();
            break;
        default:
            break;
    }
}

void WindowGUI::enter()
{
    switch (cursorl) {
        case TRACKS:
            // TODO mute/unmute
            break;
        default:
            break;
    }
}

void WindowGUI::add() 
{
    if (cursorl != SONGLIST) return;
    TRY_OOR(playUI->AddSong(songUI->GetSong()));
}

void WindowGUI::del() 
{
    if (cursorl != PLAYLIST) return;
    playUI->RemoveSong();
    TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(playUI->GetSong().GetUID()), 16));
}
