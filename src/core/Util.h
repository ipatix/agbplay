#pragma once

#include <stdexcept>
#include <cmath>

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

template<class T>
constexpr const T& clamp( const T& v, const T& lo, const T& hi )
{
	if (v < lo)
		return lo;
	if (v > hi)
		return hi;
	return v;
}

template <class T>
T sinc_pi(T x)
{
	if (x == 0)
		return 1;
	return sin(x) / x;
}
