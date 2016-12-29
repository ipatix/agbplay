#include <cstdio>
#include <cstdarg>
#include <string>
#include <mutex>
#include <cstdarg>

#include "Debug.h"

static FILE *__debug_file = nullptr;

void __print_debug(const char *str, ...) {
    va_list args;
    va_start(args, str);
    vfprintf(__debug_file, str, args);
    va_end(args);
    fflush(__debug_file);
}

bool __open_debug() {
    __debug_file = fopen("_DEBUG", "w");
    if (__debug_file == nullptr) {
        perror("fopen");
        return false;
    }
    return true;
}

bool __close_debug() {
    if (fclose(__debug_file) != 0)
        return false;
    __debug_file = nullptr;
    return true;
}

static void (*callback)(const std::string&, void *) = nullptr;
static void *cb_obj = nullptr;
static std::mutex cb_lock;

void __set_debug_callback(void (*cb)(const std::string&, void *), void *obj) {
    cb_lock.lock();
    callback = cb;
    cb_obj = obj;
    cb_lock.unlock();
}

void __print_vdebug(const char *str, ...) {
    va_list args;
    va_start(args, str);
    char txtbuf[512];
    vsnprintf(txtbuf, sizeof(txtbuf), str, args);
    va_end(args);
    cb_lock.lock();
    if (callback) {
        callback(txtbuf, cb_obj);
    }
    cb_lock.unlock();
}

void __del_debug_callback() {
    cb_lock.lock();
    callback = nullptr;
    cb_obj = nullptr;
    cb_lock.unlock();
}
