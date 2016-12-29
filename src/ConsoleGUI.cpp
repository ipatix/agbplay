#include <string>
#include <mutex>

#include "ConsoleGUI.h"
#include "Debug.h"
#include "WindowGUI.h"
#include "Xcept.h"
#include "ColorDef.h"

using namespace agbplay;
using namespace std;

/*
 * -- public --
 */

ConsoleGUI::ConsoleGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos) 
    : CursesWin(height, width, yPos, xPos) 
{
    textWidth = width - 2;
    textHeight = height;
    update();
    __set_debug_callback(remoteWrite, this);
}

ConsoleGUI::~ConsoleGUI() 
{
    __del_debug_callback();
}

void ConsoleGUI::Resize(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos) 
{
    CursesWin::Resize(height, width, yPos, xPos);
    textHeight = height;
    textWidth = width - 2;
    update();    
}

void ConsoleGUI::WriteLn(const string& str) 
{
    // shift buffer
    static mutex write_lock;

    write_lock.lock();
    writeToBuffer(str);
    update();
    write_lock.unlock();
}

/*
 * -- private --
 */

void ConsoleGUI::update() 
{
    for (uint32_t i = 0; i < textHeight; i++) 
    {
        wattrset(winPtr, COLOR_PAIR(Color::WINDOW_FRAME) | A_REVERSE);
        mvwprintw(winPtr, (int)i, 0, ">");
        wattrset(winPtr, COLOR_PAIR(Color::DEF_DEF) | A_REVERSE);
        mvwprintw(winPtr, (int)i, 1, " ");
        string txt;
        if (i < textBuffer.size()) {
            txt = textBuffer.at(i);
        } else {
            txt = "";
        }
        txt.resize(textWidth, ' ');
        mvwprintw(winPtr, (int)i, 2, txt.c_str());
    }
    wrefresh(winPtr);
}

void ConsoleGUI::writeToBuffer(const string& str) 
{
    textBuffer.push_back(str);
    if (textBuffer.size() > textHeight) {
        textBuffer.erase(textBuffer.begin() + 0);
    }
}

void ConsoleGUI::remoteWrite(const std::string& str, void *obj)
{
    ConsoleGUI *ui = (ConsoleGUI *)obj;
    ui->WriteLn(str);
}
