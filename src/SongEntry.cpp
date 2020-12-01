#include "SongEntry.h"

/*
 * SongEntry
 */

SongEntry::SongEntry(const std::string& name, uint16_t uid) 
{
    this->name = name;
    this->uid = uid;
}

SongEntry::~SongEntry() 
{
}
