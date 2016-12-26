#include <cstdio>
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
