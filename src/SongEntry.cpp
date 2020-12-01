#include "SongEntry.h"

using namespace std;

/*
 * SongEntry
 */

SongEntry::SongEntry(const string& name, uint16_t uid) 
{
    this->name = name;
    this->uid = uid;
}

SongEntry::~SongEntry() 
{
}
