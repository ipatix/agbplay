#include "FileReader.h"

#include <fstream>
#include <cstring>
#include <algorithm>

#include <zip.h>

#include "Xcept.h"

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

/* While it would be possible to avoid the 'filterFunc', it can be used to
 * avoid executing 'op', which would otherwise always force the underlying file
 * to be opened, even when not used. It is likely not a good practice to open a file
 * if it is not used (e.g. in case of missing permissions. etc).
 * It is also possibly a bit faster in the case of zip files. */

bool FileReader::forEachInZip(
        const std::filesystem::path &p,
        std::function<bool(const std::filesystem::path&)> filterFunc,
        std::function<bool(const std::filesystem::path&, FileReader &)> op)
{
    /* use unnique_ptr as RAII management for C types from libzip */
    auto errorDel = [](zip_error_t *error) { zip_error_fini(error); delete error; };
    std::unique_ptr<zip_error_t, decltype(errorDel)> error(new zip_error_t, errorDel);
    zip_error_init(error.get());

    auto sourceDel = [](zip_source_t *s){ (void)zip_source_close(s); };
    std::unique_ptr<zip_source_t, decltype(sourceDel)> source(
#if _WIN32
        zip_source_win32w_create(p.wstring().c_str(), 0, -1, error.get())
#else
        zip_source_file_create(p.string().c_str(), 0, -1, error.get())
#endif
        , sourceDel);

    if (!source)
        throw Xcept("FileReader: Unable to open zip file: {}", zip_error_strerror(error.get()));

    auto archiveDel = [](zip_t *z){ (void)zip_close(z); };
    std::unique_ptr<zip_t, decltype(archiveDel)> archive(zip_open_from_source(source.get(), ZIP_CHECKCONS | ZIP_RDONLY, error.get()), archiveDel);
    if (!archive) {
        if (zip_error_code_zip(error.get()) == ZIP_ER_NOZIP)
            return false;

        throw Xcept("FileReader: Unable to open zip file: {}", zip_error_strerror(error.get()));
    }

    const zip_int64_t numFilesInZip = zip_get_num_entries(archive.get(), 0);

    for (zip_int64_t i = 0; i < numFilesInZip; i++) {
        const char8_t *cname = reinterpret_cast<const char8_t *>(zip_get_name(archive.get(), i, 0));
        if (!cname)
            continue;
        std::u8string name(cname);
        if (name.size() == 0 || name.ends_with(u8"/"))
            continue;

        if (!filterFunc(name))
            continue;

        ZipFileReader zipFileReader(archive.get(), i);
        if (!op(name, zipFileReader))
            break;
    }

    return true;
}

void FileReader::forEachInZipOrRaw(
        const std::filesystem::path &p,
        std::function<bool(const std::filesystem::path&)> filterFunc,
        std::function<bool(const std::filesystem::path&, FileReader &)> op)
{
    if (forEachInZip(p, filterFunc, op))
        return;

    if (!filterFunc(p))
        return;

    SystemFileReader fileReader(p);
    op(p, fileReader);
}

bool FileReader::forRaw(
        const std::filesystem::path &p,
        std::function<bool(FileReader &)> op)
{
    SystemFileReader fileReader(p);
    return op(fileReader);
}

bool FileReader::cmpPathExt(const std::filesystem::path &p, const std::string &ext)
{
    std::string al = p.extension().string();
    std::string bl = "." + ext;

    std::transform(al.begin(), al.end(), al.begin(), [](char c){ return std::tolower(c); });
    std::transform(bl.begin(), bl.end(), bl.begin(), [](char c){ return std::tolower(c); });
    return al == bl;
}
