#include <string>
#include <cstdio>
#include <cstring>

#include "AgbTypes.h"
#include "Rom.h"
#include "MyException.h"
#include "Debug.h"
#include "Util.h"

using namespace agbplay;
using namespace std;

/*
 * public
 */

Rom::Rom(FileContainer& fc)
{
    data = &fc.data;
    verify();
    pos = 0;
}

Rom::~Rom() 
{
}

void Rom::Seek(long pos) 
{
    checkBounds(pos, sizeof(char));
    this->pos = pos;
}

void Rom::SeekAGBPtr(agbptr_t ptr) 
{
    long pos = ptr - AGB_MAP_ROM;
    checkBounds(pos, sizeof(char));
    this->pos = pos;
}

void Rom::RelSeek(int space)
{
    checkBounds(pos + space, sizeof(char));
    pos += space;
    return;
}

agbptr_t Rom::PosToAGBPtr(long pos) 
{
    checkBounds(pos, sizeof(char));
    return (agbptr_t)(pos + AGB_MAP_ROM);
}

long Rom::AGBPtrToPos(agbptr_t ptr) 
{
    long result = (long)ptr - AGB_MAP_ROM;
    return result;
}

int8_t Rom::ReadInt8() 
{
    checkBounds(pos, sizeof(int8_t));
    int8_t result = *(int8_t *)&(*data)[(size_t)pos];
    pos += sizeof(int8_t);
    return result;
}

uint8_t Rom::ReadUInt8() 
{
    checkBounds(pos, sizeof(int8_t));
    uint8_t result = *(uint8_t *)&(*data)[(size_t)pos];
    pos += sizeof(uint8_t);
    return result;
}

int8_t Rom::PeekInt8(int offset)
{
    return *(int8_t *)&(*data)[size_t(pos + offset)];
}

uint8_t Rom::PeekUInt8(int offset)
{
    return *(uint8_t *)&(*data)[size_t(pos + offset)];
}

int16_t Rom::ReadInt16() 
{
    checkBounds(pos, sizeof(int16_t));
    int16_t result = *(int16_t *)&(*data)[(size_t)pos];
    pos += sizeof(int16_t);
    return result;
}

uint16_t Rom::ReadUInt16() 
{
    
    checkBounds(pos, sizeof(uint16_t));
    uint16_t result = *(uint16_t *)&(*data)[(size_t)pos];
    pos += sizeof(uint16_t);
    return result;
}

int32_t Rom::ReadInt32() 
{
    checkBounds(pos, sizeof(int32_t));
    int32_t result = *(int32_t *)&(*data)[(size_t)pos];
    pos += sizeof(int32_t);
    return result;
}

uint32_t Rom::ReadUInt32() 
{
    checkBounds(pos, sizeof(uint32_t));
    uint32_t result = *(uint32_t *)&(*data)[(size_t)pos];
    pos += sizeof(uint32_t);
    return result;
}

long Rom::ReadAGBPtrToPos() 
{
    return AGBPtrToPos(ReadUInt32());
}

string Rom::ReadString(size_t limit) 
{
    if (limit > 2048)
        throw new MyException("Unable to read a string THAT long");
    string result = string((const char *)&(*data)[(size_t)pos], limit);
    pos += limit;
    return result;
}

void Rom::ReadData(void *dest, size_t bytes)
{
    checkBounds(pos, bytes);
    void *src = (void *)&(*data)[(size_t)pos];
    memcpy(dest, src, bytes);
    pos += (long)bytes;
}

uint8_t& Rom::operator[](const long oPos)
{
    checkBounds(oPos, sizeof(uint8_t));
    return (*data)[(size_t)oPos];
}

void *Rom::GetPtr() 
{
    checkBounds(pos, sizeof(char));
    return &(*data)[(size_t)pos];
}

long Rom::GetPos()
{
    return pos;
}

size_t Rom::Size() 
{
    return data->size();
}

bool Rom::ValidPointer(agbptr_t ptr) 
{
    long rec = (long)ptr - AGB_MAP_ROM;
    if (rec < 0 || rec + 4 >= (long)data->size())
        return false;
    return true;
}

string Rom::GetROMCode()
{
    Seek(0xAC);
    return ReadString(4);
}

/*
 * private
 */

void Rom::checkBounds(long pos, size_t typesz) {
    if (pos < 0 || ((size_t)pos + typesz) > data->size())
        throw MyException(FormatString("Rom Reader position out of range: %7X", pos));
}

void Rom::verify() 
{
    // check ROM size
    if (data->size() > AGB_ROM_SIZE || data->size() < 0x200)
        throw MyException("Illegal ROM size");
    
    // Logo data
    // TODO replace 1 to 1 logo comparison with checksum
    uint8_t imageBytes[] = {
        0x24,0xff,0xae,0x51,0x69,0x9a,0xa2,0x21,0x3d,0x84,0x82,0x0a,0x84,0xe4,0x09,0xad,
        0x11,0x24,0x8b,0x98,0xc0,0x81,0x7f,0x21,0xa3,0x52,0xbe,0x19,0x93,0x09,0xce,0x20,
        0x10,0x46,0x4a,0x4a,0xf8,0x27,0x31,0xec,0x58,0xc7,0xe8,0x33,0x82,0xe3,0xce,0xbf,
        0x85,0xf4,0xdf,0x94,0xce,0x4b,0x09,0xc1,0x94,0x56,0x8a,0xc0,0x13,0x72,0xa7,0xfc,
        0x9f,0x84,0x4d,0x73,0xa3,0xca,0x9a,0x61,0x58,0x97,0xa3,0x27,0xfc,0x03,0x98,0x76,
        0x23,0x1d,0xc7,0x61,0x03,0x04,0xae,0x56,0xbf,0x38,0x84,0x00,0x40,0xa7,0x0e,0xfd,
        0xff,0x52,0xfe,0x03,0x6f,0x95,0x30,0xf1,0x97,0xfb,0xc0,0x85,0x60,0xd6,0x80,0x25,
        0xa9,0x63,0xbe,0x03,0x01,0x4e,0x38,0xe2,0xf9,0xa2,0x34,0xff,0xbb,0x3e,0x03,0x44,
        0x78,0x00,0x90,0xcb,0x88,0x11,0x3a,0x94,0x65,0xc0,0x7c,0x63,0x87,0xf0,0x3c,0xaf,
        0xd6,0x25,0xe4,0x8b,0x38,0x0a,0xac,0x72,0x21,0xd4,0xf8,0x07
    };

    // check logo
    for (size_t i = 0; i < sizeof(imageBytes); i++) {
        if (imageBytes[i] != (*data)[i + 0x4])
            throw MyException("ROM verification: Bad Nintendo Logo");
    }

    // check checksum
    uint8_t checksum = (*data)[0xBD];
    int check = 0;
    for (size_t i = 0xA0; i < 0xBC; i++) {
        check -= (*data)[i];
    }
    check = (check - 0x19) & 0xFF;
    if (check != checksum)
        throw MyException(FormatString("ROM verification: Bad Header Checksum: %02X - expected %02X", (int)checksum, (int)check));
}
