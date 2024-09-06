#include "SongEntry.hpp"

/*
 * SongEntry
 */

SongEntry::SongEntry(const std::string& name, uint16_t uid) 
    : name(name), uid(uid)
{
}

const std::string& SongEntry::GetName() const
{
    return name;
}

uint16_t SongEntry::GetUID() const
{
    return uid;
}
