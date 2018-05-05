#pragma once

#include <string>

void _print_debug(const char *str, ...);
bool _open_debug();
bool _close_debug();
void _set_debug_callback(void (*cb)(const std::string&, void *), void *obj);
