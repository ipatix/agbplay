#pragma once

#include <cstdint>
#include <string>

class SongEntry
{
public:
    SongEntry(const std::string &name, uint16_t uid);

    const std::string &GetName() const;
    uint16_t GetUID() const;

    std::string name;

private:
    uint16_t uid;
};
