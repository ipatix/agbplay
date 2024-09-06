#pragma once

#include <exception>
#include <string>
#include <fmt/core.h>

class Xcept : public std::exception {
public:
    template<typename... Args>
    constexpr Xcept(fmt::format_string<Args...> fmt, Args&&... args) {
        msg = fmt::format(fmt, std::forward<Args>(args)...);
    }
    ~Xcept() override;

    const char *what() const noexcept override;
private:
    std::string msg;
};
