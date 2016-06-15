#include "Util.h"
#include "TrackviewGUI.h"
#include "ColorDef.h"
#include "Debug.h"
#include "MyException.h"

using namespace std;
using namespace agbplay;

/*
 * public
 */

TrackviewGUI::TrackviewGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos) 
: CursesWin(height, width, yPos, xPos) 
{
    // will clear the screen due to not overriding from CursesWin base class
    cursorPos = 0;
    cursorVisible = false;
    update();
}

TrackviewGUI::~TrackviewGUI() 
{
}

void TrackviewGUI::Resize(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos) 
{
    CursesWin::Resize(height, width, yPos, xPos);
    update();
}

void TrackviewGUI::SetState(const Sequence& seq, const float *vols)
{
    size_t sz;
    if (disp.data.size() != (sz = seq.tracks.size())) {
        disp.data.resize(sz);
    }
    for (size_t i = 0; i < sz; i++)
    {
        disp.data[i].trackPtr = uint32_t(seq.tracks[i].pos);
        disp.data[i].isCalling = seq.tracks[i].reptCount > 0;
        disp.data[i].isMuted = false;
        disp.data[i].vol = seq.tracks[i].vol;
        disp.data[i].mod = seq.tracks[i].mod;
        disp.data[i].prog = seq.tracks[i].prog;
        disp.data[i].pan = seq.tracks[i].pan;
        disp.data[i].pitch = seq.tracks[i].pitch;
        disp.data[i].envL = uint8_t(minmax<uint32_t>(0, uint32_t(vols[i*N_CHANNELS] * 768.f), 255));
        disp.data[i].envR = uint8_t(minmax<uint32_t>(0, uint32_t(vols[i*N_CHANNELS+1] * 768.f), 255));
        disp.data[i].delay = max((int8_t)0, seq.tracks[i].delay);
        disp.data[i].activeNotes = seq.tracks[i].activeNotes;
    }
    update();
}

void TrackviewGUI::Enter() 
{
    this->cursorVisible = true;
    update();
}

void TrackviewGUI::Leave()
{
    this->cursorVisible = false;
    update();
}

void TrackviewGUI::PageDown()
{
    for (int i = 0; i < 5; i++) {
        scrollDownNoUpdate();
    }
    update();
}

void TrackviewGUI::PageUp()
{
    for (int i = 0; i < 5; i++) {
        scrollUpNoUpdate();
    }
    update();
}

void TrackviewGUI::ScrollDown()
{
    scrollDownNoUpdate();
    update();
}

void TrackviewGUI::ScrollUp()
{
    scrollUpNoUpdate();
    update();
}

/*
 * private Trackview
 */

void TrackviewGUI::update() 
{
    // init draw
    //UIMutex.lock();
    const uint32_t yBias = 1;
    const uint32_t xBias = 1;
    // clear field
    if (cursorPos >= disp.data.size() && cursorPos > 0) {
        cursorPos = (uint32_t)disp.data.size() - 1;
    }
    string clr = "";
    clr.resize(width - xBias, ' ');
    wattrset(winPtr, COLOR_PAIR(Color::DEF_DEF));
    for (uint32_t i = yBias + 1 + (uint32_t)disp.data.size() * 2; i < height; i++) {
        mvwprintw(winPtr, (int)i, xBias, clr.c_str());
    }
    // draw borderlines
    wattrset(winPtr, COLOR_PAIR(Color::WINDOW_FRAME) | A_REVERSE);
    mvwvline(winPtr, 1, 0, ' ', height - 1);
    string titleText = " Tracker";
    titleText.resize(width, ' ');
    mvwprintw(winPtr, 0, 0, titleText.c_str());

    // draw track titlebar
    wattrset(winPtr, COLOR_PAIR(Color::DEF_DEF) | A_UNDERLINE);
    mvwprintw(winPtr, yBias, xBias, "[");
    wattrset(winPtr, COLOR_PAIR(Color::TRK_NUM) | A_UNDERLINE);
    wprintw(winPtr, "#T");
    wattrset(winPtr, COLOR_PAIR(Color::DEF_DEF) | A_UNDERLINE);
    wprintw(winPtr, "] ");
    wattrset(winPtr, COLOR_PAIR(Color::TRK_LOC) | A_UNDERLINE);
    wprintw(winPtr, "Location ");
    wattrset(winPtr, COLOR_PAIR(Color::DEF_DEF) | A_UNDERLINE);
    wprintw(winPtr, " ");
    wattrset(winPtr, COLOR_PAIR(Color::TRK_DEL) | A_UNDERLINE);
    wprintw(winPtr, "Del");
    wattrset(winPtr, COLOR_PAIR(Color::DEF_DEF) | A_UNDERLINE);
    wprintw(winPtr, " ");
    wattrset(winPtr, COLOR_PAIR(Color::TRK_NOTE) | A_UNDERLINE);
    wprintw(winPtr, "Note");
    wattrset(winPtr, COLOR_PAIR(Color::DEF_DEF) | A_UNDERLINE);
    string blank = "";
    blank.resize(width - 24, ' '); // 24 makes the line fit to screen end
    wprintw(winPtr, "%s", blank.c_str());

    for (uint32_t i = 0, th = 0; i < disp.data.size(); i++, th += 2) {
        unsigned long aFlag = (cursorVisible && i == cursorPos) ? A_REVERSE : 0;
        // print tickbox and first line
        wattrset(winPtr, COLOR_PAIR(Color::DEF_DEF) | aFlag);
        mvwprintw(winPtr, (int)(yBias + 1 + th), xBias, "[");
        wattrset(winPtr, COLOR_PAIR((disp.data[i].isMuted) ? Color::TRK_NUM_MUTED : Color::TRK_NUM) | aFlag);
        wprintw(winPtr, "%02d", i);
        wattrset(winPtr, COLOR_PAIR(Color::DEF_DEF) | aFlag);
        wprintw(winPtr, "] ");
        wattrset(winPtr, (disp.data[i].isCalling ? COLOR_PAIR(Color::TRK_LOC_CALL) : COLOR_PAIR(Color::TRK_LOC)) | aFlag);
        wprintw(winPtr, "0x%07X", disp.data[i].trackPtr);
        wattrset(winPtr, COLOR_PAIR(Color::DEF_DEF) | aFlag);
        wprintw(winPtr, " ");
        wattrset(winPtr, COLOR_PAIR(Color::TRK_DEL) | aFlag);
        wprintw(winPtr, "W%02d", disp.data[i].delay);
        wattrset(winPtr, COLOR_PAIR(Color::DEF_DEF) | aFlag);
        wprintw(winPtr, " ");
        wattrset(winPtr, COLOR_PAIR(Color::TRK_NOTE) | aFlag);
        string notes = "";
        for (uint32_t j = 0; j < disp.data[i].activeNotes.size(); j++) {
            if (disp.data[i].activeNotes[j])
                notes += noteNames[j] + " ";
        }
        notes.resize((width < 20) ? 0 : width - 20, ' ');
        wprintw(winPtr, "%s", notes.c_str());

        // print track values and sencond line
        wattrset(winPtr, COLOR_PAIR(Color::DEF_DEF) | aFlag);
        mvwprintw(winPtr, (int)(yBias + 2 + th), xBias, "    ");
        wattrset(winPtr, COLOR_PAIR(Color::TRK_VOICE) | aFlag);
        if (disp.data[i].prog == PROG_UNDEFINED) {
            wprintw(winPtr, "---");
        } else {
            wprintw(winPtr, "%-3d", disp.data[i].prog);
        }
        wattrset(winPtr, COLOR_PAIR(Color::TRK_PAN) | aFlag);
        wprintw(winPtr, " %-+3d", disp.data[i].pan);
        wattrset(winPtr, COLOR_PAIR(Color::TRK_VOL) | aFlag);
        wprintw(winPtr, " %-3d", disp.data[i].vol);
        wattrset(winPtr, COLOR_PAIR(Color::TRK_MOD) | aFlag);
        wprintw(winPtr, " %-3d", disp.data[i].mod);
        wattrset(winPtr, COLOR_PAIR(Color::TRK_PITCH) | aFlag);
        wprintw(winPtr, " %-+6d", disp.data[i].pitch);
        // print volume level
        {
            auto getFracBar = [](uint8_t frac) 
            {
                switch (frac)
                {
                    case 0:
                        return " ";
                    case 1:
                        return "\u258f";
                    case 2:
                        return "\u258e";
                    case 3:
                        return "\u258d";
                    case 4:
                        return "\u258c";
                    case 5:
                        return "\u258b";
                    case 6:
                        return "\u258a";
                    case 7:
                        return "\u2589";
                    case 8:
                        return "\u2588";
                    default:
                        throw MyException("illegal block");
                }
            };

            string bar;
            uint32_t leftBar = disp.data[i].envL / 2;
            uint32_t rightBar = disp.data[i].envR / 2;
            //assert(leftBar + rightBar + leftBlank + rightBlank == 32);
            for (size_t i = 0; i < 16; i++)
            {
                if (i < (128 - leftBar) / 8)
                    bar += "\u2588";
                else if (i == (128 - leftBar) / 8)
                    bar += getFracBar((128 - leftBar) % 8);
                else
                    bar += " ";
            }
            wattrset(winPtr, (COLOR_PAIR(Color::TRK_LOUDNESS) | A_REVERSE) ^ aFlag);
            wprintw(winPtr, "%s", bar.c_str());
            wattrset(winPtr, COLOR_PAIR(Color::TRK_LOUD_SPLIT) | aFlag);
            wprintw(winPtr, "\u2503");
            bar.clear();
            for (size_t i = 0; i < 16; i++)
            {
                if (i < rightBar / 8)
                    bar += "\u2588";
                else if (i == rightBar / 8)
                    bar += getFracBar(rightBar % 8);
                else
                    bar += " ";
            }
            wattrset(winPtr, COLOR_PAIR(Color::TRK_LOUDNESS) | aFlag);
            wprintw(winPtr, "%s", bar.c_str());
        }
        wattrset(winPtr, COLOR_PAIR(Color::DEF_DEF) | aFlag);
        string clr = "";
        clr.resize(width - 60, ' ');
        wprintw(winPtr, "%s", clr.c_str());
    }

    wrefresh(winPtr);
    //UIMutex.unlock();
}

void TrackviewGUI::scrollDownNoUpdate() 
{
    if (cursorPos + 1 < disp.data.size())
        cursorPos++;
}

void TrackviewGUI::scrollUpNoUpdate()
{
    if (cursorPos > 0)
        cursorPos--;
}

void TrackviewGUI::ForceUpdate()
{
    update();
}
