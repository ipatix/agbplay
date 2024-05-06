#pragma once

#include <stdexcept>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define PI_F (float(M_PI))

inline void CStrAppend(char *dest, size_t *index, const char *src)
{
    char ch;
    const char *copyChar = src;
    while ((ch = *copyChar++) != '\0')
        dest[(*index)++] = ch;
}
