#include <string>
#include <mutex>

#include "ConsoleGUI.h"
#include "Debug.h"
#include "WindowGUI.h"
#include "Xcept.h"
#include "ColorDef.h"

/*
 * -- public --
 */

ConsoleGUI::ConsoleGUI(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos) 
    : CursesWin(height, width, yPos, xPos), msgQueue(32)
{
    textWidth = width - 2;
    textHeight = height;
    update();
    Debug::set_callback(remoteWrite, this);
}

ConsoleGUI::~ConsoleGUI() 
{
}

void ConsoleGUI::Resize(uint32_t height, uint32_t width, uint32_t yPos, uint32_t xPos) 
{
    CursesWin::Resize(height, width, yPos, xPos);
    textHeight = height;
    textWidth = width - 2;
    update();    
}

void ConsoleGUI::Refresh() {
    std::string newText;
    bool modified = false;
    while (msgQueue.pop(newText)) {
        writeToBuffer(newText);
        modified = true;
    }
    if (modified)
        update();
}

/*
 * -- private --
 */

void ConsoleGUI::update() 
{
    for (uint32_t i = 0; i < textHeight; i++) 
    {
        wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::WINDOW_FRAME)) | A_REVERSE);
        mvwprintw(winPtr, (int)i, 0, ">");
        wattrset(winPtr, COLOR_PAIR(static_cast<int>(Color::DEF_DEF)) | A_REVERSE);
        mvwprintw(winPtr, (int)i, 1, " ");
        std::string txt;
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

void ConsoleGUI::writeToBuffer(const std::string& str) 
{
    textBuffer.push_back(str);
    if (textBuffer.size() > textHeight) {
        textBuffer.erase(textBuffer.begin() + 0);
    }
}

void ConsoleGUI::remoteWrite(const std::string& str, void *obj)
{
    ConsoleGUI *ui = (ConsoleGUI *)obj;
    ui->msgQueue.push(str);
}
