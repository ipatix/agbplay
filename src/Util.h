#pragma once

#include <stdexcept>
#include <boost/format.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

template <typename T>
inline static T clip(T min, T val, T max)
{
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

inline void CStrAppend(char *dest, size_t *index, const char *src)
{
    char ch;
    const char *copyChar = src;
    while ((ch = *copyChar++) != '\0')
        dest[(*index)++] = ch;
}
