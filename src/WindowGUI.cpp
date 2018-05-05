#include <string>
#include <algorithm>
#include <sstream>
#include <cstdlib>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <cstring>
#include <iomanip>

#include "Constants.h"
#include "Xcept.h"
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
    updateWindowSize();
    if (has_colors() == false)
        throw Xcept("Error, your terminal doesn't support colors");
    initColors();
    noecho();
    curs_set(0);
    keypad(stdscr, true);

    this->play = false;
    this->cursorl = SONGLIST;

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
    trackUI->SetTitle(songUI->GetSong().GetName());
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

bool WindowGUI::Handle() 
{
    int ch;
    while ((ch = titleUI->GetKey()) != ERR) {
        switch (ch) {
            case '\n':
                enter();
                break;
            case 18: // CTRL+R
            case KEY_RESIZE:
                updateWindowSize();
                resizeWindows();
                break;
            case KEY_UP:
            case 'k':
                scrollUp();
                break;
            case KEY_DOWN:
            case 'j':
                scrollDown();
                break;
            case KEY_LEFT:
            case 'h':
                scrollLeft();
                break;
            case KEY_RIGHT:
            case 'l':
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
            case 'T':
                if (cursorl == PLAYLIST)
                    playUI->UntickAll();
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
            case '=':
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
            case 'e':
                mplay->Stop();
                {
                    SoundExporter se(*conUI, sdata, thisCfg, rom, false, true);
                    se.Export("wav", thisCfg.GetGameEntries(), playUI->GetTicked());
                }
                break;
            case 'r':
                mplay->Stop();
                {
                    SoundExporter se(*conUI, sdata, thisCfg, rom, false, false);
                    se.Export("wav", thisCfg.GetGameEntries(), playUI->GetTicked());
                }
                break;
            case 'b':
                mplay->Stop();
                {
                    SoundExporter se(*conUI, sdata, thisCfg, rom, true, false);
                    se.Export("wav", thisCfg.GetGameEntries(), playUI->GetTicked());
                }
                break;
            case 'm':
                mute();
                break;
            case 's':
                solo();
                break;
            case 'u':
                tutti();
                break;
            case EOF:
            case 4: // EOT
            case 'q':
                _print_debug("Exiting...");
                mplay->Stop();
                return false;
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
        mplay->GetMasterVolLevels(lVol, rVol);
        meterUI->SetVol(lVol, rVol);
    }
    conUI->Refresh();
    return true;
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
        throw Xcept("Using default terminal colors failed");
    if (COLORS != 256)
        throw Xcept("Terminal does not support 256 colors");
    init_pair((int)Color::DEF_DEF, -1, -1);
    init_pair((int)Color::BANNER_TEXT, COLOR_YELLOW, -1);
    init_pair((int)Color::WINDOW_FRAME, COLOR_GREEN, -1);
    init_pair((int)Color::LIST_ENTRY, COLOR_YELLOW, -1);
    init_pair((int)Color::LIST_SEL, COLOR_RED, -1);
    init_pair((int)Color::VU_LOW, 82, -1);
    init_pair((int)Color::VU_MID, 226, -1);
    init_pair((int)Color::VU_HIGH, 202, -1);
    init_pair((int)Color::TRK_NUM, 76, -1);
    init_pair((int)Color::TRK_NUM_MUTED, 196, -1);
    init_pair((int)Color::TRK_LOC, 118, -1);
    init_pair((int)Color::TRK_LOC_CALL, 184, -1);
    init_pair((int)Color::TRK_DEL, 196, -1);
    init_pair((int)Color::TRK_NOTE, 45, -1);
    init_pair((int)Color::TRK_VOICE, 217, -1);
    init_pair((int)Color::TRK_PAN, 214, -1);
    init_pair((int)Color::TRK_VOL, 154, -1);
    init_pair((int)Color::TRK_MOD, 43, -1);
    init_pair((int)Color::TRK_PITCH, 129, -1);
    init_pair((int)Color::TRK_LOUDNESS, 70, /*238*/-1);
    init_pair((int)Color::TRK_LOUDNESS_MUTED, 166, /*238*/-1);
    init_pair((int)Color::TRK_LOUD_SPLIT, -1, /*238*/-1);

    init_pair((int)Color::TRK_FGB_BGCW, 232, 251);
    init_pair((int)Color::TRK_FGC_BGCW, 198, 251);
    init_pair((int)Color::TRK_FGB_BGW, 232, 255);
    init_pair((int)Color::TRK_FGC_BGW, 198, 255);
    init_pair((int)Color::TRK_FGB_BGC, 232, 198);
    init_pair((int)Color::TRK_FGC_BGC, 198, 198);
    init_pair((int)Color::TRK_FGW_BGW, 255, 255);
    init_pair((int)Color::TRK_FGW_BGC, 255, 198);
}

void WindowGUI::cycleFocus() 
{
    switch (cursorl) {
        case SONGLIST:
            songUI->Leave();
            cursorl = PLAYLIST;
            playUI->Enter();
            try {
                mplay->LoadSong(sdata.sTable->GetPosOfSong(playUI->GetSong().GetUID()));
                trackUI->SetTitle(playUI->GetSong().GetName());
            } catch (const std::out_of_range& e) { }
            break;
        case PLAYLIST:
            playUI->Leave();
            cursorl = SONGLIST;
            songUI->Enter();
            try {
                mplay->LoadSong(sdata.sTable->GetPosOfSong(songUI->GetSong().GetUID()));
                trackUI->SetTitle(songUI->GetSong().GetName());
            } catch (const std::out_of_range& e) { }
            break;
        default:
            break;
    }
}

void WindowGUI::scrollLeft()
{
    switch (cursorl) {
        case TRACKS_SONGLIST:
            trackUI->Leave();
            cursorl = SONGLIST;
            songUI->Enter();
            break;
        case TRACKS_PLAYLIST:
            trackUI->Leave();
            cursorl = PLAYLIST;
            playUI->Enter();
        default:
            break;
    }
}

void WindowGUI::scrollRight()
{
    switch (cursorl) {
        case SONGLIST:
            songUI->Leave();
            cursorl = TRACKS_SONGLIST;
            trackUI->Enter();
            break;
        case PLAYLIST:
            playUI->Leave();
            cursorl = TRACKS_PLAYLIST;
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
            try {
                mplay->LoadSong(sdata.sTable->GetPosOfSong(songUI->GetSong().GetUID()));
                trackUI->SetTitle(songUI->GetSong().GetName());
            } catch (const std::out_of_range& e) { }
            break;
        case PLAYLIST:
            playUI->ScrollDown();
            if (!playUI->IsDragging()) {
                try {
                    mplay->LoadSong(sdata.sTable->GetPosOfSong(playUI->GetSong().GetUID()));
                    trackUI->SetTitle(playUI->GetSong().GetName());
                } catch (const std::out_of_range& e) { }
            }
            break;
        case TRACKS_SONGLIST:
        case TRACKS_PLAYLIST:
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
            try {
                mplay->LoadSong(sdata.sTable->GetPosOfSong(songUI->GetSong().GetUID()));
                trackUI->SetTitle(songUI->GetSong().GetName());
            } catch (const std::out_of_range& e) { }
            break;
        case PLAYLIST:
            playUI->ScrollUp();
            if (!playUI->IsDragging()) {
                try {
                    mplay->LoadSong(sdata.sTable->GetPosOfSong(playUI->GetSong().GetUID()));
                    trackUI->SetTitle(playUI->GetSong().GetName());
                } catch (const std::out_of_range& e) { }
            }
            break;
        case TRACKS_SONGLIST:
        case TRACKS_PLAYLIST:
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
            try {
                mplay->LoadSong(sdata.sTable->GetPosOfSong(songUI->GetSong().GetUID()));
                trackUI->SetTitle(songUI->GetSong().GetName());
            } catch (const std::out_of_range& e) { }
            break;
        case PLAYLIST:
            playUI->PageDown();
            if (!playUI->IsDragging()) {
                try {
                    mplay->LoadSong(sdata.sTable->GetPosOfSong(playUI->GetSong().GetUID()));
                    trackUI->SetTitle(playUI->GetSong().GetName());
                } catch (const std::out_of_range& e) { }
            }
            break;
        case TRACKS_SONGLIST:
        case TRACKS_PLAYLIST:
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
            try {
                mplay->LoadSong(sdata.sTable->GetPosOfSong(songUI->GetSong().GetUID()));
                trackUI->SetTitle(songUI->GetSong().GetName());
            } catch (const std::out_of_range& e) { }
            break;
        case PLAYLIST:
            playUI->PageUp();
            if (!playUI->IsDragging()) {
                try {
                    mplay->LoadSong(sdata.sTable->GetPosOfSong(playUI->GetSong().GetUID()));
                    trackUI->SetTitle(playUI->GetSong().GetName());
                } catch (const std::out_of_range& e) { }
            }
            break;
        case TRACKS_SONGLIST:
        case TRACKS_PLAYLIST:
            trackUI->PageUp();
            break;
        default:
            break;
    }
}

void WindowGUI::enter()
{
    switch (cursorl) {
        case TRACKS_SONGLIST:
        case TRACKS_PLAYLIST:
            // TODO mute/unmute
            mplay->ToggleMute(trackUI->GetCursorLoc());
            break;
        default:
            break;
    }
}

void WindowGUI::add() 
{
    if (cursorl != SONGLIST) 
        return;

    try {
        playUI->AddSong(songUI->GetSong());
    } catch (const std::out_of_range& e) { }
}

void WindowGUI::del() 
{
    if (cursorl != PLAYLIST) 
        return;
    playUI->RemoveSong();
    try {
        mplay->LoadSong(sdata.sTable->GetPosOfSong(playUI->GetSong().GetUID()));
        trackUI->SetTitle(playUI->GetSong().GetName());
    } catch (const std::out_of_range& e) { }
}

void WindowGUI::mute()
{
    size_t cur;
    size_t count;
    switch (cursorl) {
        case TRACKS_SONGLIST:
        case TRACKS_PLAYLIST:
            cur = trackUI->GetCursorLoc();
            count = mplay->GetMaxTracks();
            for (size_t i = 0; i < count; i++) {
                if (cur == i)
                    mplay->Mute(i, true);
            }
            break;
        default:
            break;
    }
}

void WindowGUI::solo()
{
    size_t cur;
    size_t count;
    switch (cursorl) {
        case TRACKS_SONGLIST:
        case TRACKS_PLAYLIST:
            cur = trackUI->GetCursorLoc();
            count = mplay->GetMaxTracks();
            for (size_t i = 0; i < count; i++) {
                mplay->Mute(i, cur != i);
            }
            break;
        default:
            break;
    }
}

void WindowGUI::tutti()
{
    size_t count;
    switch (cursorl) {
        case TRACKS_SONGLIST:
        case TRACKS_PLAYLIST:
            count = mplay->GetMaxTracks();
            for (size_t i = 0; i < count; i++) {
                mplay->Mute(i, false);
            }
            break;
        default:
            break;
    }
}

void WindowGUI::rename()
{
    if (cursorl != PLAYLIST) 
        return;

    SongEntry *ent;
    try {
        ent = &playUI->GetSong();
    } catch (const std::out_of_range& e) {
        return;
    }
    const int renHeight = 5;
    const int renWidth = 40;
    WINDOW *renWin = newwin(5, 40, (height / 2) - (renHeight / 2), (width / 2) - (renWidth / 2));
    keypad(renWin, true);
    curs_set(1);
    if (renWin == nullptr)
        throw Xcept("Error creating renaming window");
    wattrset(renWin, COLOR_PAIR(Color::DEF_DEF) | A_REVERSE);
    mvwhline(renWin, 0, 0, ' ', renWidth);
    // pls no unicode in title or size() below requires fix
    string title = "New Name";
    string line = " \u250c";
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
        throw Xcept("Error while deleting renaming window");
    }
}

void WindowGUI::updateWindowSize()
{
    getmaxyx(stdscr, height, width);
    width = clip(WINDOW_MIN_WIDTH, width, WINDOW_MAX_WIDTH);
    height = clip(WINDOW_MIN_HEIGHT, height, WINDOW_MAX_HEIGHT);
}
