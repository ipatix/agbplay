#include "Rom.hpp"

#include <string>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <optional>
#include <algorithm>

#include <zip.h>

#include "Xcept.hpp"
#include "Debug.hpp"
#include "Util.hpp"
#include "Gsf.hpp"
#include "FileReader.hpp"

std::unique_ptr<Rom> Rom::globalInstance;

/*
 * public
 */

Rom Rom::LoadFromFile(const std::filesystem::path &filePath)
{
    Rom rom;
    rom.LoadFile(filePath);
    rom.romData = rom.romContainer;
    rom.Verify();
    return rom;
}

Rom Rom::LoadFromBufferCopy(std::span<uint8_t> buffer)
{
    Rom rom;
    rom.romContainer.assign(buffer.begin(), buffer.end());
    rom.romData = rom.romContainer;
    rom.Verify();
    return rom;
}

Rom Rom::LoadFromBufferRef(std::span<uint8_t> buffer)
{
    Rom rom;
    rom.romData = buffer;
    rom.Verify();
    return rom;
}

void Rom::CreateInstance(const std::filesystem::path& filePath)
{
    globalInstance = std::make_unique<Rom>(LoadFromFile(filePath));
}

std::string Rom::ReadString(size_t pos, size_t limit) const
{
    std::string result;
    for (size_t i = 0; i < limit; i++) {
        char c = static_cast<char>(ReadU8(pos + i));
        if (c == '\0')
            break;
        result += c;
    }
    return result;
}

std::string Rom::GetROMCode() const
{
    std::string result = ReadString(0xAC, 4);
    if (!IsGsf() || !result.empty())
        return result;
    return Gsf::GuessGameCodeFromPath(gsfPath);
}

bool Rom::IsGsf() const
{
    return isGsf;
}

/*
 * private
 */

void Rom::Verify()
{
    // check ROM size
    if (romData.size() > AGB_ROM_SIZE || romData.size() < 0x200)
        throw Xcept("Illegal ROM size");

    if (isGsf)
        return;
    
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
        if (imageBytes[i] != ReadU8(i + 0x4))
            throw Xcept("ROM verification: Bad Nintendo Logo");
    }

    // check checksum
    uint8_t checksum = ReadU8(0xBD);
    int check = 0;
    for (size_t i = 0xA0; i < 0xBD; i++) {
        check -= ReadU8(i);
    }
    check = (check - 0x19) & 0xFF;
    if (check != checksum)
        throw Xcept("ROM verification: Bad Header Checksum: {:02X} - expected {:02X}", checksum, check);
}

void Rom::LoadFile(const std::filesystem::path& filePath)
{
    /* Try load load as zip file */
    if (LoadZip(filePath))
        return;
    if (LoadGsflib(filePath))
        return;
    if (LoadRaw(filePath))
        return;
    throw Xcept("Unable to determine input file type");
}

bool Rom::LoadZip(const std::filesystem::path &filePath)
{
    auto filterFunc = [](const std::filesystem::path &p) {
        if (FileReader::cmpPathExt(p, "gba"))
            return true;
        if (FileReader::cmpPathExt(p, "gsflib"))
            return true;
        return false;
    };

    bool fileInZipLoaded = false;

    auto op = [this, &fileInZipLoaded](const std::filesystem::path &p, FileReader &fileReader) {
        if (FileReader::cmpPathExt(p, "gba")) {
            fileInZipLoaded = LoadRaw(fileReader);
            return false;
        }
        if (FileReader::cmpPathExt(p, "gsflib")) {
            fileInZipLoaded = LoadGsflib(fileReader);
            return false;
        }

        /* If the filterFunc works correctly, we should not reach this case. However,
         * worst case we will just continue iterating over the next file until it matches. */
        assert(false);
        return true;
    };

    /* return false if the zip file is not a zip file. */
    if (!FileReader::forEachInZip(filePath, filterFunc, op)) {
        assert(!fileInZipLoaded);
        return false;
    }

    /* return true of the zip file was loaded and a valid file was found */
    if (fileInZipLoaded)
        return true;

    /* throw an error if no valid files were found in the zip file */
    throw Xcept("LoadZip: Unable to locate .gsflib or .gba file in zip file");
}

bool Rom::LoadGsflib(const std::filesystem::path& filePath)
{
    return FileReader::forRaw(filePath, [this](FileReader &fileReader) {
            return LoadGsflib(fileReader);
    });
}

bool Rom::LoadGsflib(FileReader &fileReader)
{
    std::vector<uint8_t> gsfData(fileReader.size());
    fileReader.read(gsfData);
    isGsf = true;
    gsfPath = fileReader.path();
    return Gsf::GetRomData(gsfData, romContainer);
}

bool Rom::LoadRaw(const std::filesystem::path& filePath)
{
    return FileReader::forRaw(filePath, [this](FileReader &fileReader) {
            return LoadRaw(fileReader);
    });
}

bool Rom::LoadRaw(FileReader &fileReader)
{
    /* get file size */
    const size_t size = fileReader.size();
    if (size <= 0x200)
        throw Xcept("ERROR: Attempting to load tiny ROM (<= 512 bytes). Incorrect or damaged file?");
    if (size > AGB_ROM_SIZE)
        throw Xcept("ERROR: Attempting to illegally large ROM (> 32 MiB). Incorrect or damaged file?");

    /* read exactly as much data as the file contains */
    romContainer.resize(size);
    fileReader.read(romContainer);
    isGsf = false;
    return true;
}
