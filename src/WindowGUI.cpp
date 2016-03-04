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
#include "SoundExporter.h"

#define KEY_TAB 9

using namespace agbplay;
using namespace std;

WindowGUI::WindowGUI(Rom& rrom, SoundData& rsdata) 
    : rom(rrom), sdata(rsdata), cfg("agbplay.ini"), thisCfg(cfg.GetConfig(rom.GetROMCode()))
{
    // init ncurses stuff
    this->containerWin = initscr();
    getmaxyx(stdscr, height, width);
    if (has_colors() == false)
        throw MyException("Error, your terminal doesn't support colors");
    initColors();
    noecho();
    curs_set(0);
    keypad(stdscr, true);

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

    playUI = new PlaylistGUI(
            PLAYLIST_HEIGHT(height, width),
            PLAYLIST_WIDTH(height, width),
            PLAYLIST_YPOS(height, width),
            PLAYLIST_XPOS(height, width),
            thisCfg);

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
    mplay = new PlayerInterface(rom, trackUI, rom.ReadAGBPtrToPos(), thisCfg);
    mplay->LoadSong(sdata.sTable->GetPosOfSong(0));
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
                    play = true;
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
                case 'n':
                    playUI->Leave();
                    rename();
                    trackUI->ForceUpdate();
                    playUI->Enter();
                    break;
                case 'r':
                    mplay->Stop();
                    {
                        SoundExporter se(*conUI, sdata, thisCfg, rom, false);
                        se.Export("wav", thisCfg.GetGameEntries(), playUI->GetTicked());
                    }
                    break;
                case 'b':
                    mplay->Stop();
                    {
                        SoundExporter se(*conUI, sdata, thisCfg, rom, true);
                        se.Export("wav", thisCfg.GetGameEntries(), playUI->GetTicked());
                    }
                    break;
                case EOF:
                case 4: // EOT
                case 'q':
                    conUI->WriteLn("Exiting...");
                    mplay->Stop();
                    return;
            } // end key handling switch
        } // end key loop
        if (play) {
            if (!mplay->IsPlaying()) {
                if (cursorl != PLAYLIST && cursorl != SONGLIST) {
                    play = false;
                } else {
                    scrollDown();
                    mplay->Play();
                }
            }
            mplay->UpdateView();
            float lVol;
            float rVol;
            mplay->GetVolLevels(lVol, rVol);
            meterUI->SetVol(lVol, rVol);
        }
        this_thread::sleep_for(chrono::milliseconds(16));
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
    start_color();
    if (use_default_colors() == ERR)
        throw MyException("Using default terminal colors failed");
    if (COLORS != 256)
        throw MyException("Terminal does not support 256 colors");
    init_pair((int)Color::DEF_DEF, -1, -1);
    init_pair((int)Color::BANNER_TEXT, COLOR_YELLOW, -1);
    init_pair((int)Color::WINDOW_FRAME, COLOR_GREEN, -1);
    init_pair((int)Color::LIST_ENTRY, COLOR_YELLOW, -1);
    init_pair((int)Color::LIST_SEL, COLOR_RED, -1);
    init_pair((int)Color::VU_LOW, 82, -1);
    init_pair((int)Color::VU_MID, 226, -1);
    init_pair((int)Color::VU_HIGH, 202, -1);
    init_pair((int)Color::TRK_NUM, 76, -1);
    init_pair((int)Color::TRK_NUM_MUTED, 202, -1);
    init_pair((int)Color::TRK_LOC, 118, -1);
    init_pair((int)Color::TRK_LOC_CALL, 184, -1);
    init_pair((int)Color::TRK_DEL, 196, -1);
    init_pair((int)Color::TRK_NOTE, 45, -1);
    init_pair((int)Color::TRK_VOICE, 217, -1);
    init_pair((int)Color::TRK_PAN, 214, -1);
    init_pair((int)Color::TRK_VOL, 154, -1);
    init_pair((int)Color::TRK_MOD, 43, -1);
    init_pair((int)Color::TRK_PITCH, 129, -1);
}

void WindowGUI::cycleFocus() 
{
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
            TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(songUI->GetSong().GetUID())));
            break;
        case PLAYLIST:
            playUI->ScrollDown();
            TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(playUI->GetSong().GetUID())));
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
            TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(songUI->GetSong().GetUID())));
            break;
        case PLAYLIST:
            playUI->ScrollUp();
            TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(playUI->GetSong().GetUID())));
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
            TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(songUI->GetSong().GetUID())));
            break;
        case PLAYLIST:
            playUI->PageDown();
            TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(playUI->GetSong().GetUID())));
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
            TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(songUI->GetSong().GetUID())));
            break;
        case PLAYLIST:
            playUI->PageUp();
            TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(playUI->GetSong().GetUID())));
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
    TRY_OOR(mplay->LoadSong(sdata.sTable->GetPosOfSong(playUI->GetSong().GetUID())));
}

void WindowGUI::rename()
{
    if (cursorl != PLAYLIST) return;
    SongEntry *ent;
    try {
        ent = &playUI->GetSong();
    } catch (exception& e) {
        return;
    }
    const int renHeight = 5;
    const int renWidth = 40;
    WINDOW *renWin = newwin(5, 40, (height / 2) - (renHeight / 2), (width / 2) - (renWidth / 2));
    keypad(renWin, true);
    curs_set(1);
    if (renWin == nullptr)
        throw MyException("Error creating renaming window");
    wattrset(renWin, COLOR_PAIR(Color::DEF_DEF) | A_REVERSE);
    string line = "";
    line.resize(renWidth, ' ');
    mvwprintw(renWin, 0, 0, "%s", line.c_str());
    // pls no unicode in title or size() below requires fix
    string title = "New Name";
    line = " \u250c";
    string leftBar = "";
    string rightBar = "";
    for (int i = 0; i < (renWidth - (int)title.size())/2 - 3; i++) 
    {
        leftBar.append("\u2500");
    }
    for (int i = 0; i < (renWidth - (int)title.size())/2 + (renWidth - (int)title.size())%2 - 3; i++)
    {
        rightBar.append("\u2500");
    }
    line.append(leftBar);
    line.append(" ");
    line.append(title);
    line.append(" ");
    line.append(rightBar);
    line.append("\u2510 ");
    mvwprintw(renWin, 1, 0, "%s", line.c_str());
    mvwprintw(renWin, 2, 0, " \u2502 ");
    wattrset(renWin, COLOR_PAIR(Color::DEF_DEF));
    line = "";
    line.resize(renWidth - 6, ' ');
    wprintw(renWin, "%s", line.c_str());
    wattrset(renWin, COLOR_PAIR(Color::DEF_DEF) | A_REVERSE);
    wprintw(renWin, " \u2502 ");
    line = " \u2514";
    for (int i = 0; i < renWidth - 4; i++)
    {
        line.append("\u2500");
    }
    line.append("\u2518 ");
    mvwprintw(renWin, 3, 0, "%s", line.c_str());
    line = "";
    line.resize(renWidth, ' ');
    mvwprintw(renWin, 4, 0, "%s", line.c_str());

    // finished drawing windows, now read the user input
    wattrset(renWin, COLOR_PAIR(Color::DEF_DEF));
    char inputBuf[renWidth - 6];
    echo();
    if (mvwgetnstr(renWin, 2, 3, inputBuf, sizeof(inputBuf) - 1) != ERR) {
        ent->name = string(inputBuf, strlen(inputBuf));
    }
    noecho();
    curs_set(0);
    if (delwin(renWin) == ERR) {
        throw MyException("Error while deleting renaming window");
    }
}
