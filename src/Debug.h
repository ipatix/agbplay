#pragma once

#include <string>

void __print_debug(const char *str, ...);
bool __open_debug();
bool __close_debug();

void __print_vdebug(const char *str, ...);
void __set_debug_callback(void (*cb)(const std::string&, void *), void *obj);
void __del_debug_callback();
