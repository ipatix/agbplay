#pragma once

#include <stdexcept>
#include <boost/format.hpp>

#define TRY_OOR(n) try { n ; } catch (std::out_of_range& e) { }

namespace agbplay
{
    template <typename T>
        inline static T minmax(T min, T val, T max)
        {
            if (val < min) return min;
            if (val > max) return max;
            return val;
        }

    template <typename T>
        inline static T min(T a, T b)
        {
            return a < b ? a : b;
        }

    template <typename T>
        inline static T max(T a, T b)
        {
            return a > b ? a : b;
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
}
