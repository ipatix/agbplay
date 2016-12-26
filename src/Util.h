#pragma once

#include <stdexcept>
#include <boost/format.hpp>

template <typename T>
inline static T clip(T min, T val, T max)
{
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

inline static std::string FormatStringRecurse(boost::format& message)
{
    return message.str();
}

template <typename TValue, typename... TArgs>
std::string FormatStringRecurse(boost::format& message, TValue&& arg, TArgs&&... args)
{
    message % std::forward<TValue>(arg);
    return FormatStringRecurse(message, std::forward<TArgs>(args)...);
}

template <typename... TArgs>
std::string FormatString(const char* fmt, TArgs&&... args)
{
    using namespace boost::io;
    boost::format message(fmt);
    return FormatStringRecurse(message, std::forward<TArgs>(args)...);
}

inline void CStrAppend(char *dest, size_t *index, const char *src)
{
    char ch;
    const char *copyChar = src;
    while ((ch = *copyChar++) != '\0')
        dest[(*index)++] = ch;
}
