#include "SongEntry.h"

using namespace std;
using namespace agbplay;

/*
 * SongEntry
 */

SongEntry::SongEntry(string name, uint16_t uid) 
{
    this->name = name;
    this->uid = uid;
}

SongEntry::~SongEntry() 
{
}

uint16_t SongEntry::GetUID() 
{
    return uid;
}
