#include "WindowGUI.hpp"

#include "ColorDef.hpp"
#include "Constants.hpp"
#include "Debug.hpp"
#include "Settings.hpp"
#include "SoundExporter.hpp"
#include "Util.hpp"
#include "Xcept.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

#define KEY_TAB 9

WindowGUI::WindowGUI(Profile &profile) : profile(profile)
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
    conUI = std::make_unique<ConsoleGUI>(
        CONSOLE_HEIGHT(height, width),
        CONSOLE_WIDTH(height, width),
        CONSOLE_YPOS(height, width),
        CONSOLE_XPOS(height, width)
    );

    hotUI = std::make_unique<HotkeybarGUI>(
        HOTKEYBAR_HEIGHT(height, width),
        HOTKEYBAR_WIDTH(height, width),
        HOTKEYBAR_YPOS(height, width),
        HOTKEYBAR_XPOS(height, width)
    );

    songUI = std::make_unique<SonglistGUI>(
        SONGLIST_HEIGHT(height, width),
        SONGLIST_WIDTH(height, width),
        SONGLIST_YPOS(height, width),
        SONGLIST_XPOS(height, width),
        true
    );

    // add songs to table
    assert(profile.songTableInfoPlayback.count != SongTableInfo::COUNT_AUTO);
    for (uint16_t i = 0; i < profile.songTableInfoPlayback.count; i++) {
        auto songName = fmt::format("{:04}", i);
        songUI->AddSong(Profile::PlaylistEntry{songName, i});
    }
    songUI->Enter();

    playUI = std::make_unique<PlaylistGUI>(
        PLAYLIST_HEIGHT(height, width),
        PLAYLIST_WIDTH(height, width),
        PLAYLIST_YPOS(height, width),
        PLAYLIST_XPOS(height, width)
    );

    for (const Profile::PlaylistEntry &entry : profile.playlist)
        playUI->AddSong(entry);

    titleUI = std::make_unique<TitlebarGUI>(
        TITLEBAR_HEIGHT(height, width),
        TITLEBAR_WIDTH(height, width),
        TITLEBAR_YPOS(height, width),
        TITLEBAR_XPOS(height, width)
    );

    romUI = std::make_unique<RomviewGUI>(
        ROMVIEW_HEIGHT(height, width),
        ROMVIEW_WIDTH(height, width),
        ROMVIEW_YPOS(height, width),
        ROMVIEW_XPOS(height, width),
        profile.songTableInfoPlayback
    );

    trackUI = std::make_unique<TrackviewGUI>(
        TRACKVIEW_HEIGHT(height, width),
        TRACKVIEW_WIDTH(height, width),
        TRACKVIEW_YPOS(height, width),
        TRACKVIEW_XPOS(height, width)
    );

    meterUI = std::make_unique<VUMeterGUI>(
        VUMETER_HEIGHT(height, width),
        VUMETER_WIDTH(height, width),
        VUMETER_YPOS(height, width),
        VUMETER_XPOS(height, width)
    );

    settings.Load();

    mplay = std::make_unique<PlaybackEngine>(settings.playbackSampleRate, profile);

    profile.dirty = true;
    trackUI->SetTitle("0000");
}

WindowGUI::~WindowGUI()
{
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
        case 18:    // CTRL+R
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
            play = mplay->Pause();
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
            if (auto *entry = playUI->GetSong(); entry != nullptr)
                trackUI->SetTitle(entry->name);
            trackUI->ForceUpdate();
            playUI->Enter();
            break;
        case 'e':
            if (exportReady())
                exportLaunch(false, true);
            break;
        case 'r':
            if (exportReady())
                exportLaunch(false, false);
            break;
        case 'b':
            if (exportReady())
                exportLaunch(true, false);
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
        case 'f':
            Debug::print("Saving will take place on close.");
            save();
            break;
        case '!':
            songInfo();
            break;
        case EOF:
        case 4:    // EOT
        case 'q':
        case 27:    // Escape Key
            // don't allow closing if an export is still running
            if (!exportReady())
                break;

            Debug::print("Exiting...");
            save();
            mplay->Stop();
            return false;
        }    // end key handling switch
    }    // end key loop

    if (play && mplay->SongEnded()) {
        if ((cursorl != PLAYLIST && cursorl != SONGLIST) || isLastSong()) {
            play = false;
            mplay->Stop();
        } else {
            scrollDown();
            mplay->Play();
        }
    }

    updateVisualizerState();
    conUI->Refresh();
    return true;
}

void WindowGUI::resizeWindows()
{
    conUI->Resize(
        CONSOLE_HEIGHT(height, width),
        CONSOLE_WIDTH(height, width),
        CONSOLE_YPOS(height, width),
        CONSOLE_XPOS(height, width)
    );
    hotUI->Resize(
        HOTKEYBAR_HEIGHT(height, width),
        HOTKEYBAR_WIDTH(height, width),
        HOTKEYBAR_YPOS(height, width),
        HOTKEYBAR_XPOS(height, width)
    );
    songUI->Resize(
        SONGLIST_HEIGHT(height, width),
        SONGLIST_WIDTH(height, width),
        SONGLIST_YPOS(height, width),
        SONGLIST_XPOS(height, width)
    );
    playUI->Resize(
        PLAYLIST_HEIGHT(height, width),
        PLAYLIST_WIDTH(height, width),
        PLAYLIST_YPOS(height, width),
        PLAYLIST_XPOS(height, width)
    );
    titleUI->Resize(
        TITLEBAR_HEIGHT(height, width),
        TITLEBAR_WIDTH(height, width),
        TITLEBAR_YPOS(height, width),
        TITLEBAR_XPOS(height, width)
    );
    romUI->Resize(
        ROMVIEW_HEIGHT(height, width),
        ROMVIEW_WIDTH(height, width),
        ROMVIEW_YPOS(height, width),
        ROMVIEW_XPOS(height, width)
    );
    trackUI->Resize(
        TRACKVIEW_HEIGHT(height, width),
        TRACKVIEW_WIDTH(height, width),
        TRACKVIEW_YPOS(height, width),
        TRACKVIEW_XPOS(height, width)
    );
    meterUI->Resize(
        VUMETER_HEIGHT(height, width),
        VUMETER_WIDTH(height, width),
        VUMETER_YPOS(height, width),
        VUMETER_XPOS(height, width)
    );
}

void WindowGUI::initColors()
{
    start_color();

    short defFg = -1, defBg = -1;
    if (use_default_colors() == ERR) {
        // if this call could have been successful -1 would be the valid default color
        defFg = COLOR_WHITE;
        defBg = COLOR_BLACK;
    }

    if (COLORS != 256)
        throw Xcept("Terminal does not support 256 colors");

    init_pair((int)Color::DEF_DEF, defFg, defBg);
    init_pair((int)Color::BANNER_TEXT, COLOR_YELLOW, defBg);
    init_pair((int)Color::WINDOW_FRAME, COLOR_GREEN, defBg);
    init_pair((int)Color::LIST_ENTRY, COLOR_YELLOW, defBg);
    init_pair((int)Color::LIST_SEL, COLOR_RED, defBg);
    init_pair((int)Color::VU_LOW, 82, defBg);
    init_pair((int)Color::VU_MID, 226, defBg);
    init_pair((int)Color::VU_HIGH, 202, defBg);
    init_pair((int)Color::TRK_NUM, 76, defBg);
    init_pair((int)Color::TRK_NUM_MUTED, 196, defBg);
    init_pair((int)Color::TRK_LOC, 118, defBg);
    init_pair((int)Color::TRK_LOC_CALL, 184, defBg);
    init_pair((int)Color::TRK_DEL, 196, defBg);
    init_pair((int)Color::TRK_NOTE, 45, defBg);
    init_pair((int)Color::TRK_VOICE, 217, defBg);
    init_pair((int)Color::TRK_PAN, 214, defBg);
    init_pair((int)Color::TRK_VOL, 154, defBg);
    init_pair((int)Color::TRK_MOD, 43, defBg);
    init_pair((int)Color::TRK_PITCH, 129, defBg);
    init_pair((int)Color::TRK_LOUDNESS, 70, /*238*/ defBg);
    init_pair((int)Color::TRK_LOUDNESS_MUTED, 166, /*238*/ defBg);
    init_pair((int)Color::TRK_LOUD_SPLIT, defFg, /*238*/ defBg);

    init_pair((int)Color::TRK_FGB_BGCW, 232, 251);
    init_pair((int)Color::TRK_FGC_BGCW, 161, 251);
    init_pair((int)Color::TRK_FGB_BGW, 232, 255);
    init_pair((int)Color::TRK_FGC_BGW, 161, 255);
    init_pair((int)Color::TRK_FGB_BGC, 232, 199);
    init_pair((int)Color::TRK_FGC_BGC, 161, 199);
    init_pair((int)Color::TRK_FGW_BGW, 255, 255);
    init_pair((int)Color::TRK_FGW_BGC, 255, 199);
    init_pair((int)Color::TRK_FGEC_BGW, 199, 255);
    init_pair((int)Color::TRK_FGEC_BGC, 199, 199);
}

void WindowGUI::cycleFocus()
{
    switch (cursorl) {
    case SONGLIST:
        songUI->Leave();
        cursorl = PLAYLIST;
        playUI->Enter();
        if (auto *entry = playUI->GetSong(); entry != nullptr)
            loadSong(*entry);
        break;
    case PLAYLIST:
        playUI->Leave();
        cursorl = SONGLIST;
        songUI->Enter();
        if (auto *entry = songUI->GetSong(); entry != nullptr)
            loadSong(*entry);
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
        if (auto *entry = songUI->GetSong(); entry != nullptr)
            loadSong(*entry);
        break;
    case PLAYLIST:
        playUI->ScrollDown();
        if (!playUI->IsDragging()) {
            if (auto *entry = playUI->GetSong(); entry != nullptr)
                loadSong(*entry);
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
        if (auto *entry = songUI->GetSong(); entry != nullptr)
            loadSong(*entry);
        break;
    case PLAYLIST:
        playUI->ScrollUp();
        if (playUI->IsDragging())
            break;
        if (auto *entry = playUI->GetSong(); entry != nullptr)
            loadSong(*entry);
        break;
    case TRACKS_SONGLIST:
    case TRACKS_PLAYLIST:
        trackUI->ScrollUp();
        break;
    default:
        break;
    }
}

bool WindowGUI::isLastSong() const
{
    switch (cursorl) {
    case SONGLIST:
        return songUI->IsLast();
    case PLAYLIST:
        return playUI->IsLast();
    default:
        return false;
    }
}

void WindowGUI::pageDown()
{
    switch (cursorl) {
    case SONGLIST:
        songUI->PageDown();
        if (auto *entry = songUI->GetSong(); entry != nullptr)
            loadSong(*entry);
        break;
    case PLAYLIST:
        playUI->PageDown();
        if (playUI->IsDragging())
            break;
        if (auto *entry = playUI->GetSong(); entry != nullptr)
            loadSong(*entry);
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
        if (auto *entry = songUI->GetSong(); entry != nullptr)
            loadSong(*entry);
        break;
    case PLAYLIST:
        playUI->PageUp();
        if (playUI->IsDragging())
            break;
        if (auto *entry = playUI->GetSong(); entry != nullptr)
            loadSong(*entry);
        break;
    case TRACKS_SONGLIST:
    case TRACKS_PLAYLIST:
        trackUI->PageUp();
        break;
    default:
        break;
    }
}

void WindowGUI::songInfo()
{
    Profile::PlaylistEntry *entry;

    if (cursorl == SONGLIST) {
        entry = songUI->GetSong();
    } else if (cursorl == PLAYLIST) {
        entry = playUI->GetSong();
    } else {
        return;
    }

    if (entry == nullptr) {
        Debug::print("Cannot get song info: No song loaded!");
        return;
    }

    SongInfo sinfo = mplay->GetSongInfo();

    Debug::print(
        "Song Info: num={} header=0x{:X} voicetable=0x{:X} reverbSet={} reverbLevel={} priority={} playerIdx={}",
        entry->id,
        sinfo.songHeaderPos,
        sinfo.voiceTablePos,
        sinfo.reverb >> 7,
        sinfo.reverb & 0x7F,
        sinfo.priority,
        sinfo.playerIdx
    );
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

    auto *entry = songUI->GetSong();
    if (entry != nullptr)
        playUI->AddSong(*entry);
}

void WindowGUI::del()
{
    if (cursorl != PLAYLIST)
        return;
    playUI->RemoveSong();
    if (auto *entry = playUI->GetSong(); entry != nullptr)
        loadSong(*entry);
}

void WindowGUI::mute()
{
    if (cursorl != TRACKS_SONGLIST && cursorl != TRACKS_PLAYLIST)
        return;

    mplay->Mute(trackUI->GetCursorLoc(), true);
}

void WindowGUI::solo()
{
    if (cursorl != TRACKS_SONGLIST && cursorl != TRACKS_PLAYLIST)
        return;

    for (size_t i = 0; i < MAX_TRACKS; i++)
        mplay->Mute(i, trackUI->GetCursorLoc() != i);
}

void WindowGUI::tutti()
{
    if (cursorl != TRACKS_SONGLIST && cursorl != TRACKS_PLAYLIST)
        return;

    for (size_t i = 0; i < MAX_TRACKS; i++)
        mplay->Mute(i, false);
}

void WindowGUI::rename()
{
    if (cursorl != PLAYLIST)
        return;

    auto *ent = playUI->GetSong();
    if (ent == nullptr)
        return;

    const int renHeight = 5;
    const int renWidth = 60;
    WINDOW *renWin = newwin(renHeight, renWidth, (height / 2) - (renHeight / 2), (width / 2) - (renWidth / 2));
    keypad(renWin, true);
    curs_set(1);
    if (renWin == nullptr)
        throw Xcept("Error creating renaming window");
    wattrset(renWin, COLOR_PAIR(static_cast<int>(Color::DEF_DEF)) | A_REVERSE);
    mvwhline(renWin, 0, 0, ' ', renWidth);
    // pls no unicode in title or size() below requires fix
    std::string title = "New Name";
    std::string line = " \u250c";
    std::string leftBar = "";
    std::string rightBar = "";
    for (int i = 0; i < (renWidth - (int)title.size()) / 2 - 3; i++) {
        leftBar.append("\u2500");
    }
    for (int i = 0; i < (renWidth - (int)title.size()) / 2 + (renWidth - (int)title.size()) % 2 - 3; i++) {
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
    wattrset(renWin, COLOR_PAIR(static_cast<int>(Color::DEF_DEF)));
    line = "";
    line.resize(renWidth - 6, ' ');
    wprintw(renWin, "%s", line.c_str());
    wattrset(renWin, COLOR_PAIR(static_cast<int>(Color::DEF_DEF)) | A_REVERSE);
    wprintw(renWin, " \u2502 ");
    line = " \u2514";
    for (int i = 0; i < renWidth - 4; i++) {
        line.append("\u2500");
    }
    line.append("\u2518 ");
    mvwprintw(renWin, 3, 0, "%s", line.c_str());
    line = "";
    line.resize(renWidth, ' ');
    mvwprintw(renWin, 4, 0, "%s", line.c_str());

    // finished drawing windows, now read the user input
    wattrset(renWin, COLOR_PAIR(static_cast<int>(Color::DEF_DEF)));
    char inputBuf[renWidth - 6];
    echo();
    if (mvwgetnstr(renWin, 2, 3, inputBuf, sizeof(inputBuf) - 1) != ERR) {
        if (strlen(inputBuf) > 0) {
            // only change the name if it's not blank
            ent->name = std::string(inputBuf, strlen(inputBuf));
        }
    }
    noecho();
    curs_set(0);
    if (delwin(renWin) == ERR) {
        throw Xcept("Error while deleting renaming window");
    }
}

void WindowGUI::save()
{
    profile.playlist = playUI->GetSongs();
    profile.dirty = true;
}

void WindowGUI::updateWindowSize()
{
    getmaxyx(stdscr, height, width);
    width = std::clamp(width, WINDOW_MIN_WIDTH, WINDOW_MAX_WIDTH);
    height = std::clamp(height, WINDOW_MIN_HEIGHT, WINDOW_MAX_HEIGHT);
}

void WindowGUI::updateVisualizerState()
{
    mplay->GetVisualizerState(visualizerState);
    meterUI->SetVol(visualizerState.masterRmsLeft, visualizerState.masterRmsRight);
    trackUI->SetState(visualizerState);
}

void WindowGUI::loadSong(const Profile::PlaylistEntry &entry)
{
    mplay->LoadSong(entry.id);
    trackUI->SetTitle(entry.name);

    if (play)
        mplay->Play();
}

void WindowGUI::exportLaunch(bool benchmarkOnly, bool separate)
{
    const auto &ticked = playUI->GetTicked();
    assert(profile.playlist.size() == ticked.size());

    /* We have to pass a copy of the profile to the other thread to that we do not
     * get race conditions when modifying the profile e.g. in the playlist editor. */

    Profile profileToExport = profile;
    profileToExport.playlist.clear();
    for (size_t i = 0; i < profile.playlist.size(); i++) {
        if (ticked[i])
            profileToExport.playlist.emplace_back(profile.playlist[i]);
    }

    exportBusy.store(true);

    exportThread = std::make_unique<std::thread>(
        [this](std::filesystem::path dir, Settings tSettings, Profile profile, bool tBenchmarkOnly, bool tSeparate) {
            SoundExporter se(dir, tSettings, profile, tBenchmarkOnly, tSeparate);
            se.Export();
            exportBusy.store(false);
        },
        settings.exportQuickExportDirectory,
        settings,
        profileToExport,
        benchmarkOnly,
        separate
    );
}

bool WindowGUI::exportReady()
{
    if (exportBusy.load()) {
        Debug::print("Please wait until the current export is finished");
        return false;
    }

    if (exportThread) {
        exportThread->join();
        exportThread.reset();
    }

    return true;
}
