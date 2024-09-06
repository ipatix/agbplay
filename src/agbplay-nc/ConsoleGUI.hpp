#pragma once

#define CONSOLE_BORDER_WIDTH 2

#include <vector>
#include <string>
#include <mutex>

#include <boost/lockfree/spsc_queue.hpp>

#include "CursesWin.hpp"

class ConsoleGUI : public CursesWin {
public:
    ConsoleGUI(uint32_t height, uint32_t width, 
            uint32_t yPos, uint32_t xPos);
    ~ConsoleGUI() override;
    void Resize(uint32_t height, uint32_t width,
            uint32_t yPos, uint32_t xPos) override;
    void Refresh();
private:
    void update() override;
    void writeToBuffer(const std::string& str);
    static void remoteWrite(const std::string& str, void *obj);

    uint32_t textWidth, textHeight;
    std::vector<std::string> textBuffer;
    boost::lockfree::spsc_queue<std::string> msgQueue;
    std::mutex writeMutex;
};
