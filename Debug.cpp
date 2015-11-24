#include <cstdio>

static FILE *__debug_file = nullptr;

void __print_debug(const char *text) {
    fprintf(__debug_file, "%s\n", text);
    fflush(__debug_file);
}

void __print_pointer(void *p) {
    fprintf(__debug_file, "Pointer: %p\n", p);
    fflush(__debug_file);
}

bool __open_debug() {
    __debug_file = fopen("DEBUG", "w");
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
