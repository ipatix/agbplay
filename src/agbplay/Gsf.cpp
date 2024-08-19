#include "Gsf.h"

#include <sstream>
#include <algorithm>
#include <memory>
#include <regex>
#include <cassert>

#include <zlib.h>

#include "Xcept.h"

static void Decompress(std::span<const uint8_t> compressedData, std::vector<uint8_t> &decompressedData)
{
    int error = Z_OK;;
    const size_t CHUNK_SIZE = 4096;
    decompressedData.clear();

    z_stream strm{};
    strm.next_in = reinterpret_cast<Bytef *>(const_cast<uint8_t *>(compressedData.data()));
    strm.avail_in = static_cast<unsigned int>(compressedData.size());

    if (error = inflateInit(&strm); error != Z_OK)
        throw Xcept("inflateInit() failed: {}", zError(error));

    /* abuse std::unique_ptr for RAII management of stack variable strm */
    auto streamDel = [](z_stream *strm) { inflateEnd(strm); };
    std::unique_ptr<z_stream, decltype(streamDel)> strmRaii(&strm, streamDel);

    while (true) {
        decompressedData.resize(decompressedData.size() + CHUNK_SIZE);
        strm.next_out = reinterpret_cast<Bytef *>(&decompressedData[strm.total_out]);
        strm.avail_out = CHUNK_SIZE;

        error = inflate(&strm, Z_NO_FLUSH); 
        if (error == Z_STREAM_END)
            break;
        else if (error != Z_OK)
            throw Xcept("inflate() failed: {}", zError(error));
    }

    decompressedData.resize(strm.total_out);
}

static bool ReadGsfData(std::span<const uint8_t> gsfData, std::vector<uint8_t> &reservedData, std::vector<uint8_t> &programData, std::string &tagData)
{
    if (gsfData.size() < 16)
        return false;

    const uint8_t GSF_VERSION_BYTE = 0x22;
    if (gsfData[0] != 'P' || gsfData[1] != 'S' || gsfData[2] != 'F' || gsfData[3] != GSF_VERSION_BYTE)
        return false;

    const size_t compReservedSize = (gsfData[4] << 0) | (gsfData[5] << 8) | (gsfData[6] << 16) | (gsfData[7] << 24);
    const size_t compProgramSize = (gsfData[8] << 0) | (gsfData[9] << 8) | (gsfData[10] << 16) | (gsfData[11] << 24);
    const uint32_t compProgramCrc32 = (gsfData[12] << 0) | (gsfData[13] << 8) | (gsfData[14] << 16) | (gsfData[15] << 24);

    if (gsfData.size() < (16 + compReservedSize + compProgramSize))
        throw Xcept("ReadGsfData(): ill-formed gsflib, size in header larger than available");

    size_t dataOffset = 16;
    if (compReservedSize > 0) {
        Decompress(gsfData.subspan(dataOffset, compReservedSize), reservedData);
        dataOffset += compReservedSize;
    } else {
        reservedData.clear();
    }

    unsigned long crc = crc32_z(0, nullptr, 0);
    crc = crc32_z(crc, reinterpret_cast<const Bytef *>(&gsfData[dataOffset]), compProgramSize);
    if (crc != compProgramCrc32)
        throw Xcept("ReadGsfData(): program data crc32 mismatch: expected={:#08x} calculated={:#08x}", compProgramCrc32, crc);

    Decompress(gsfData.subspan(dataOffset, compProgramSize), programData);
    dataOffset += compProgramSize;

    size_t tagDataSize = gsfData.size() - dataOffset;
    if (tagDataSize > 50000)
        tagDataSize = 50000; // do I understand Neill Corlett's doc right that this is limited to 50k bytes?

    tagData.assign(reinterpret_cast<const char *>(&gsfData[dataOffset]), tagDataSize);
    return true;
}

bool Gsf::GetRomData(std::span<const uint8_t> gsfData, std::vector<uint8_t> &resultRomData)
{
    std::vector<uint8_t> reservedData;
    std::string tagData;
    if (!ReadGsfData(gsfData, reservedData, resultRomData, tagData)) {
        resultRomData.clear();
        return false;
    }

    if (resultRomData.size() < 12)
        throw Xcept("Gsf::GetRomData(): program data is ill-formed (size < 12)");

    const size_t entryPoint = (resultRomData[0] << 0) | (resultRomData[1] << 8) | (resultRomData[2] << 16) | (resultRomData[3] << 24);
    const size_t offset = (resultRomData[4] << 0) | (resultRomData[5] << 8) | (resultRomData[6] << 16) | (resultRomData[7] << 24);
    const size_t romSize = (resultRomData[8] << 0) | (resultRomData[9] << 8) | (resultRomData[10] << 16) | (resultRomData[11] << 24);

    // currently unused
    (void)entryPoint;
    (void)offset;

    resultRomData.erase(resultRomData.begin(), resultRomData.begin() + 12);

    if (resultRomData.size() != romSize)
        throw Xcept("Gsf::GetRomData(): size mismatch between delcared ROM size and decompressed size");

    // uncomment this for GSF debugging and examination with hex editor
    //std::ofstream ofs("/tmp/gsf.gba");
    //if (ofs.is_open()) {
    //    ofs.write(reinterpret_cast<const char *>(resultRomData.data()), resultRomData.size());
    //    ofs.close();
    //}

    return true;
}

void Gsf::GetSongInfo(std::span<const uint8_t> gsfData, std::string &name, uint16_t &id)
{
    std::vector<uint8_t> programData;
    std::vector<uint8_t> reservedData;
    std::string tagData;
    if (!ReadGsfData(gsfData, reservedData, programData, tagData))
        throw Xcept("Gsf::GetSongInfo(): file does not appear to be a minigsf file");

    if (programData.size() == 13) {
        id = programData[12];
    } else if (programData.size() == 14) {
        id = static_cast<uint16_t>(programData[12] | (programData[13] << 8));
    } else {
        throw Xcept("Gsf::GetSongInfo(): program data of minigsf neither 12 nor 13 bytes.");
    }

    if (!tagData.starts_with("[TAG]"))
        throw Xcept("Gsf::GetSongInfo(): no tag data found in minigsf");
    tagData = tagData.substr(5);

    /* read all tags and locate title */
    std::stringstream ss(tagData);
    std::string line;

    name.clear();

    while (std::getline(ss, line)) {
        std::string lowerCaseLine = line;
        std::transform(lowerCaseLine.begin(), lowerCaseLine.end(), lowerCaseLine.begin(), [](char c){ return std::tolower(c); });
        if (lowerCaseLine.starts_with("title=")) {
            name = line.substr(6);
            break;
        }
    }

    if (name.empty())
        throw Xcept("Gsf::GetSongInfo(): unable to find title in minigsf tags");
}

std::string Gsf::GuessGameCodeFromPath(const std::filesystem::path &p)
{
    /* This is not unicode safe, but game codes are 4 byte ASCII anyway,
     * so it ought to be good enough for identifying a GSF. */
    std::string filename = p.stem().string();
    std::regex re("^AGB-([A-Z0-9]{4})-\\w+$"); // example: AGB-AN8J-JPN
    std::smatch matches;

    if (std::regex_match(filename, matches, re)) {
        assert(matches.size() == 2);
        return matches[1].str();
    } else {
        return "";
    }
}
