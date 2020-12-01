#pragma once

#include <cstdint>
#include <string>

class SongEntry 
{
private:
    uint16_t uid;
public:
    SongEntry(const std::string& name, uint16_t uid);
    ~SongEntry();

    std::string name;
    const std::string& GetName() const { return name; };
    uint16_t GetUID() const { return uid; };
};
