#pragma once

#include <exception>
#include <format>
#include <string>

class Xcept : public std::exception
{
public:
    template<typename... Args> constexpr Xcept(std::format_string<Args...> fmt, Args &&...args)
    {
        msg = std::format(fmt, std::forward<Args>(args)...);
    }
    ~Xcept() override;

    const char *what() const noexcept override;

private:
    std::string msg;
};
