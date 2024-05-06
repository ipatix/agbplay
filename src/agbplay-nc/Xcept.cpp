#include <cstdarg>

#include "Xcept.h"

Xcept::Xcept(const char *format, ...) {
    va_list args;
    va_start(args, format);
    char msg_buf[MAX_EXCEPTION_LENGTH];
    vsnprintf(msg_buf, MAX_EXCEPTION_LENGTH, format, args);
    msg = msg_buf;
    va_end(args);
}

Xcept::~Xcept() {
}

const char *Xcept::what() const throw() {
    return msg.c_str();
}
