#include "Xcept.h"

Xcept::~Xcept() { }

const char *Xcept::what() const noexcept {
    return msg.c_str();
}
