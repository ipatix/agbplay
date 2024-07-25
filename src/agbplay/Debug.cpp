#include <cstdio>
#include <cstdarg>
#include <string>
#include <mutex>
#include <cstdarg>

#include "Debug.h"

static FILE *debug_file = nullptr;

bool Debug::open(const char *file) {
    if (!file)
        return true;

    debug_file = fopen(file, "w");
    if (debug_file == nullptr) {
        perror("fopen");
        return false;
    }
    return true;
}

bool Debug::close() {
    if (debug_file == nullptr)
        return true;

    if (fclose(debug_file) != 0)
        return false;
    debug_file = nullptr;
    return true;
}


static void (*callback)(const std::string&, void *) = nullptr;
static void *cb_obj = nullptr;

void Debug::set_callback(void (*cb)(const std::string&, void *), void *obj) {
    callback = cb;
    cb_obj = obj;
}

void Debug::puts(const std::string &msg) {
    if (debug_file) {
        fprintf(debug_file, "%s\n", msg.c_str());
        fflush(debug_file);
    }

    if (callback)
        callback(msg.c_str(), cb_obj);
}