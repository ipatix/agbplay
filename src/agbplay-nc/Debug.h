#pragma once

#include <string>
#include <fmt/core.h>

namespace Debug {
    void puts(const std::string &msg);
    template<typename... Args>
    void print(fmt::format_string<Args...> fmt, Args&&... args) {
        Debug::puts(fmt::format(fmt, std::forward<Args>(args)...));
    }
    bool open(const char *file);
    bool close();
    void set_callback(void (*cb)(const std::string&, void *), void *obj);
}
