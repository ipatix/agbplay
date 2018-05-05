#include <cstdio>
#include <cstdarg>
#include <string>
#include <mutex>
#include <cstdarg>

#include "Debug.h"

static FILE *debug_file = nullptr;

bool _open_debug() {
    debug_file = fopen("_DEBUG.txt", "w");
    if (debug_file == nullptr) {
        perror("fopen");
        return false;
    }
    return true;
}

bool _close_debug() {
    if (fclose(debug_file) != 0)
        return false;
    debug_file = nullptr;
    return true;
}


static void (*callback)(const std::string&, void *) = nullptr;
static void *cb_obj = nullptr;

void _set_debug_callback(void (*cb)(const std::string&, void *), void *obj) {
    callback = cb;
    cb_obj = obj;
}

void _print_debug(const char *str, ...) {
    va_list args;
    va_start(args, str);
    char txtbuf[512];
    vsnprintf(txtbuf, sizeof(txtbuf), str, args);
    fprintf(debug_file, "%s\n", txtbuf);
    fflush(debug_file);
    va_end(args);
    if (callback) {
        callback(txtbuf, cb_obj);
    }
}
