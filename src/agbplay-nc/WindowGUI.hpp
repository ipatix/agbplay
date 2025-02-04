#pragma once

/* window size definition
 * (h,w) = height and width of stdscr */

#define TITLEBAR_HEIGHT(h, w) 5
#define TITLEBAR_WIDTH(h, w)  33
#define TITLEBAR_YPOS(h, w)   0
#define TITLEBAR_XPOS(h, w)   0

#define VUMETER_HEIGHT(h, w) TITLEBAR_HEIGHT(h, w)
#define VUMETER_WIDTH(h, w)  (w - TITLEBAR_WIDTH(h, w))
#define VUMETER_YPOS(h, w)   0
#define VUMETER_XPOS(h, w)   TITLEBAR_WIDTH(h, w)

#define SONGLIST_HEIGHT(h, w) ((h - TITLEBAR_HEIGHT(h, w)) / 2)
#define SONGLIST_WIDTH(h, w)  25
#define SONGLIST_YPOS(h, w)   (TITLEBAR_HEIGHT(h, w))
#define SONGLIST_XPOS(h, w)   0

#define PLAYLIST_HEIGHT(h, w) ((h - TITLEBAR_HEIGHT(h, w)) / 2 + ((h - TITLEBAR_HEIGHT(h, w)) % 2))
#define PLAYLIST_WIDTH(h, w)  SONGLIST_WIDTH(h, w)
#define PLAYLIST_YPOS(h, w)   (TITLEBAR_HEIGHT(h, w) + ((h - TITLEBAR_HEIGHT(h, w)) / 2))
#define PLAYLIST_XPOS(h, w)   0

#define CONSOLE_HEIGHT(h, w) 6
#define CONSOLE_WIDTH(h, w)  (w - SONGLIST_WIDTH(h, w))
#define CONSOLE_YPOS(h, w)   (h - CONSOLE_HEIGHT(h, w))
#define CONSOLE_XPOS(h, w)   (SONGLIST_WIDTH(h, w))

#define HOTKEYBAR_HEIGHT(h, w) 1
#define HOTKEYBAR_WIDTH(h, w)  (w - SONGLIST_WIDTH(h, w))
#define HOTKEYBAR_YPOS(h, w)   (h - CONSOLE_HEIGHT(h, w) - HOTKEYBAR_HEIGHT(h, w))
#define HOTKEYBAR_XPOS(h, w)   SONGLIST_WIDTH(h, w)

#define ROMVIEW_HEIGHT(h, w) (h - TITLEBAR_HEIGHT(h, w) - HOTKEYBAR_HEIGHT(h, w) - CONSOLE_HEIGHT(h, w))
#define ROMVIEW_WIDTH(h, w)  20
#define ROMVIEW_YPOS(h, w)   TITLEBAR_HEIGHT(h, w)
#define ROMVIEW_XPOS(h, w)   (w - ROMVIEW_WIDTH(h, w))

#define TRACKVIEW_HEIGHT(h, w) (h - TITLEBAR_HEIGHT(h, w) - HOTKEYBAR_HEIGHT(h, w) - CONSOLE_HEIGHT(h, w))
#define TRACKVIEW_WIDTH(h, w)  (w - SONGLIST_WIDTH(h, w) - ROMVIEW_WIDTH(h, w))
#define TRACKVIEW_YPOS(h, w)   TITLEBAR_HEIGHT(h, w)
#define TRACKVIEW_XPOS(h, w)   SONGLIST_WIDTH(h, w)

// various macros

#define CONTROL(x) (x & 0x1F)

#include "ConsoleGUI.hpp"
#include "HotkeybarGUI.hpp"
#include "PlaybackEngine.hpp"
#include "PlaylistGUI.hpp"
#include "Profile.hpp"
#include "RomviewGUI.hpp"
#include "Settings.hpp"
#include "SonglistGUI.hpp"
#include "TitlebarGUI.hpp"
#include "TrackviewGUI.hpp"
#include "Types.hpp"
#include "VUMeterGUI.hpp"

#include <memory>

class WindowGUI
{
public:
    WindowGUI(Profile &profile);
    WindowGUI(const WindowGUI &) = delete;
    WindowGUI &operator=(const WindowGUI &) = delete;
    ~WindowGUI();

    bool Handle();

private:
    // sub window control
    void resizeWindows();
    // color definitions
    void initColors();
    // hotkey methods
    void cycleFocus();
    void scrollLeft();
    void scrollRight();
    void scrollDown();
    void scrollUp();
    bool isLastSong() const;
    void pageDown();
    void pageUp();
    void songInfo();
    void enter();
    void add();
    void del();
    void mute();
    void solo();
    void tutti();
    void rename();
    void save();

    void updateWindowSize();
    void updateVisualizerState();
    void loadSong(const Profile::PlaylistEntry &entry);

    void exportLaunch(bool benchmarkOnly, bool separate);
    bool exportReady();

    // console GUI element
    std::unique_ptr<ConsoleGUI> conUI;
    std::unique_ptr<HotkeybarGUI> hotUI;
    std::unique_ptr<SonglistGUI> songUI;
    std::unique_ptr<PlaylistGUI> playUI;
    std::unique_ptr<TitlebarGUI> titleUI;
    std::unique_ptr<RomviewGUI> romUI;
    std::unique_ptr<TrackviewGUI> trackUI;
    std::unique_ptr<VUMeterGUI> meterUI;

    // resource
    std::unique_ptr<PlaybackEngine> mplay;
    std::unique_ptr<std::thread> exportThread;
    std::atomic<bool> exportBusy = false;

    // other
    Profile &profile;
    MP2KVisualizerState visualizerState;
    Settings settings;

    // ncurses windows
    WINDOW *containerWin;

    int width, height;
    bool play;
    enum { PLAYLIST, SONGLIST, TRACKS_PLAYLIST, TRACKS_SONGLIST, SETTINGS } cursorl;
};
