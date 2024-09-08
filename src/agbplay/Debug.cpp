#include "Debug.hpp"

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <format>
#include <iostream>
#include <mutex>
#include <string>

static FILE *debug_file = nullptr;

bool Debug::open(const char *file)
{
    if (!file)
        return true;

    debug_file = fopen(file, "w");
    if (debug_file == nullptr) {
        perror("fopen");
        return false;
    }
    return true;
}

bool Debug::close()
{
    if (debug_file == nullptr)
        return true;

    if (fclose(debug_file) != 0)
        return false;
    debug_file = nullptr;
    return true;
}

static void (*callback)(const std::string &, void *) = nullptr;
static void *cb_obj = nullptr;

void Debug::set_callback(void (*cb)(const std::string &, void *), void *obj)
{
    callback = cb;
    cb_obj = obj;
}

void Debug::puts(const std::string &msg)
{
    /* not sure why this requires std::format instead of fmt::format */
    std::string finalMsg = std::format("[{:%T}] {}", std::chrono::system_clock::now(), msg);

    if (debug_file) {
        fprintf(debug_file, "%s\n", finalMsg.c_str());
        fflush(debug_file);
    }

    if (callback)
        callback(finalMsg.c_str(), cb_obj);
    else
        std::cout << finalMsg << std::endl;
}
