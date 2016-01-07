#pragma once

#include "Events.h"

/* window size definition
 * (h,w) = height and width of stdscr */

#define TITLEBAR_HEIGHT(h,w) 5
#define TITLEBAR_WIDTH(h,w) w
#define TITLEBAR_YPOS(h,w) 0
#define TITLEBAR_XPOS(h,w) 0

#define SONGLIST_HEIGHT(h,w) ((h - TITLEBAR_HEIGHT(h,w)) / 2)
#define SONGLIST_WIDTH(h,w) 25
#define SONGLIST_YPOS(h,w) (TITLEBAR_HEIGHT(h,w))
#define SONGLIST_XPOS(h,w) 0

#define PLAYLIST_HEIGHT(h,w) ((h - TITLEBAR_HEIGHT(h,w)) / 2 + ((h - TITLEBAR_HEIGHT(h,w)) % 2))
#define PLAYLIST_WIDTH(h,w) SONGLIST_WIDTH(h,w)
#define PLAYLIST_YPOS(h,w) (TITLEBAR_HEIGHT(h,w) + ((h - TITLEBAR_HEIGHT(h,w)) / 2))
#define PLAYLIST_XPOS(h,w) 0

#define CONSOLE_HEIGHT(h,w) 6
#define CONSOLE_WIDTH(h,w) (w - SONGLIST_WIDTH(h,w))
#define CONSOLE_YPOS(h,w) (h - CONSOLE_HEIGHT(h,w))
#define CONSOLE_XPOS(h,w) (SONGLIST_WIDTH(h,w))

#define HOTKEYBAR_HEIGHT(h,w) 1
#define HOTKEYBAR_WIDTH(h,w) (w - SONGLIST_WIDTH(h,w))
#define HOTKEYBAR_YPOS(h,w) (h - CONSOLE_HEIGHT(h,w) - HOTKEYBAR_HEIGHT(h,w))
#define HOTKEYBAR_XPOS(h,w) SONGLIST_WIDTH(h,w)

#define ROMVIEW_HEIGHT(h,w) (h - TITLEBAR_HEIGHT(h,w) - HOTKEYBAR_HEIGHT(h,w) - CONSOLE_HEIGHT(h,w))
#define ROMVIEW_WIDTH(h,w) 20
#define ROMVIEW_YPOS(h,w) TITLEBAR_HEIGHT(h,w)
#define ROMVIEW_XPOS(h,w) (w - ROMVIEW_WIDTH(h,w))

#define TRACKVIEW_HEIGHT(h,w) (h - TITLEBAR_HEIGHT(h,w) - HOTKEYBAR_HEIGHT(h,w) - CONSOLE_HEIGHT(h,w))
#define TRACKVIEW_WIDTH(h,w) (w - SONGLIST_WIDTH(h,w) - ROMVIEW_WIDTH(h,w))
#define TRACKVIEW_YPOS(h,w) TITLEBAR_HEIGHT(h,w)
#define TRACKVIEW_XPOS(h,w) SONGLIST_WIDTH(h,w)

// various macros

#define CONTROL(x) (x & 0x1F)


#include <ncurses.h>

#include "Rom.h"
#include "ConsoleGUI.h"
#include "HotkeybarGUI.h"
#include "SonglistGUI.h"
#include "PlaylistGUI.h"
#include "TitlebarGUI.h"
#include "RomviewGUI.h"
#include "TrackviewGUI.h"
#include "PlayerInterface.h"

namespace agbplay 
{
    class WindowGUI 
    {
        public:
            WindowGUI(Rom& rrom, SoundData& rsdata);
            ~WindowGUI();

            // main GUI handler
            void Handle();
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
            void pageDown();
            void pageUp();
            void enter();
            void add();
            void del();
            void toggle();

            // console GUI element
            ConsoleGUI *conUI;
            HotkeybarGUI *hotUI;
            SonglistGUI *songUI;
            PlaylistGUI *playUI;
            TitlebarGUI *titleUI;
            RomviewGUI *romUI;
            TrackviewGUI *trackUI;
            Events *event;

            // resource
            Rom& rom;
            SoundData& sdata;
            PlayerInterface *mplay;

            // ncurses windows
            WINDOW *containerWin;
            
            uint32_t width, height;
            enum {
                PLAYLIST, SONGLIST, TRACKS, SETTINGS
            } cursorl;
    }; // end WindowGUI
} // end namespace agbplay
