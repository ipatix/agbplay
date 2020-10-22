#pragma once

#include <string>

void print_debug(const char *str, ...);
bool open_debug(const char *file);
bool close_debug();
void set_debug_callback(void (*cb)(const std::string&, void *), void *obj);
