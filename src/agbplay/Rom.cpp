#include <string>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <optional>
#include <algorithm>

#include <zip.h>

#include "Rom.h"
#include "Xcept.h"
#include "Debug.h"
#include "Util.h"
#include "Gsf.h"

std::unique_ptr<Rom> Rom::globalInstance;

class FileReader {
public:
    virtual ~FileReader() = default;
    [[maybe_unused]] virtual void read(std::span<uint8_t> buffer) = 0;
    virtual size_t size() = 0;
    virtual void close() = 0;
};

class SystemFileReader : public FileReader {
public:
    SystemFileReader(const std::filesystem::path &filePath) : ifs(filePath, std::ios_base::binary) {
        if (!ifs.is_open())
            throw Xcept("Error opening file (path={}): {}", filePath.string(), strerror(errno));
    }
    ~SystemFileReader() override = default;

    [[maybe_unused]] void read(std::span<uint8_t> buffer) override {
        ifs.read(reinterpret_cast<char *>(buffer.data()), buffer.size());
        if (ifs.bad())
            throw Xcept("Could not read file: bad bit is set");
    }
    size_t size() {
        ifs.seekg(0, std::ios_base::end);
        const std::ifstream::pos_type fileEnd = ifs.tellg();
        if (fileEnd == -1)
            throw Xcept("Error seeking in input file");
        ifs.seekg(0, std::ios_base::beg);
        return static_cast<size_t>(fileEnd);
    }
    void close() {
        ifs.close();
    }

private:
    std::ifstream ifs;
};

class ZipFileReader : public FileReader {
public:
    ZipFileReader(zip_t *archive, zip_uint64_t index) {
        zip_stat_init(&s);
        if (zip_stat_index(archive, index, 0, &s) == -1)
            throw Xcept("zip_stat_index() failed: {}", zip_strerror(archive));

        file = zip_fopen_index(archive, index, 0);
        if (!file)
            throw Xcept("zip_fopen_index() failed: {}", zip_strerror(archive));
    }
    ~ZipFileReader() override {
        close();
    }

    [[maybe_unused]] void read(std::span<uint8_t> buffer) override {
        const zip_int64_t bytesRead = zip_fread(file, static_cast<void *>(buffer.data()), buffer.size());
        if (bytesRead == 0)
            throw Xcept("zip_fread(): end of file reached");
        if (bytesRead == -1)
            throw Xcept("zip_fread(): error occured while reading: {}", zip_file_strerror(file));
    }
    size_t size() {
        return static_cast<size_t>(s.size);
    }
    void close() {
        if (file) {
            zip_fclose(file);
            file = nullptr;
        }
    }

private:
    zip_stat_t s;
    zip_file_t *file = nullptr;
};

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
    return ReadString(0xAC, 4);
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
    /* use unnique_ptr as RAII management for C types */
    auto errorDel = [](zip_error_t *error) { zip_error_fini(error); delete error; };
    std::unique_ptr<zip_error_t, decltype(errorDel)> error(new zip_error_t, errorDel);
    zip_error_init(error.get());

    auto sourceDel = [](zip_source_t *s){ (void)zip_source_close(s); };
    std::unique_ptr<zip_source_t, decltype(sourceDel)> source(
#if _WIN32
        zip_source_win32w_create(filePath.wstring().c_str(), 0, -1, error.get())
#else
        zip_source_file_create(filePath.string().c_str(), 0, -1, error.get())
#endif
        , sourceDel);

    if (!source)
        throw Xcept("LoadZip: Unable to open zip file: {}", zip_error_strerror(error.get()));

    auto archiveDel = [](zip_t *z){ (void)zip_close(z); };
    std::unique_ptr<zip_t, decltype(archiveDel)> archive(zip_open_from_source(source.get(), ZIP_CHECKCONS | ZIP_RDONLY, error.get()), archiveDel);
    if (!archive) {
        if (zip_error_code_zip(error.get()) == ZIP_ER_NOZIP)
            return false;

        throw Xcept("LoadZip: Unable to open zip file: {}", zip_error_strerror(error.get()));
    }

    zip_int64_t numFilesInZip = zip_get_num_entries(archive.get(), 0);

    for (zip_int64_t i = 0; i < numFilesInZip; i++) {
        const char *cname = zip_get_name(archive.get(), i, 0);
        if (!cname)
            continue;
        std::string name(cname);
        if (name.size() == 0 || name.ends_with("/"))
            continue;

        std::transform(name.begin(), name.end(), name.begin(), [](char c) { return std::tolower(c); });

        if (name.ends_with(".gba")) {
            ZipFileReader zipFileReader(archive.get(), i);
            return LoadRaw(zipFileReader);
        }

        if (name.ends_with(".gsflib")) {
            ZipFileReader zipFileReader(archive.get(), i);
            return LoadGsflib(zipFileReader);
        }
    }

    throw Xcept("LoadZip: Unable to locate .gsflib or .gba file in zip file");
}

bool Rom::LoadGsflib(const std::filesystem::path& filePath)
{
    SystemFileReader systemFileReader(filePath);
    return LoadGsflib(systemFileReader);
}

bool Rom::LoadGsflib(FileReader &fileReader)
{
    std::vector<uint8_t> gsfData(fileReader.size());
    fileReader.read(gsfData);
    isGsf = true;
    return Gsf::GetRomData(gsfData, romContainer);
}

bool Rom::LoadRaw(const std::filesystem::path& filePath)
{
    SystemFileReader systemFileReader(filePath);
    return LoadRaw(systemFileReader);
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
