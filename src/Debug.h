#pragma once

#include <string>

namespace Debug {
    void print(const char *str, ...);
    bool open(const char *file);
    bool close();
    void set_callback(void (*cb)(const std::string&, void *), void *obj);
}
