#pragma once

#include <span>
#include <cstdint>
#include <cstddef>
#include <functional>
#include <filesystem>

class FileReader {
public:
    virtual ~FileReader() = default;
    [[maybe_unused]] virtual void read(std::span<uint8_t> buffer) = 0;
    virtual size_t size() = 0;
    virtual void close() = 0;
    virtual std::filesystem::path path() const = 0;

    [[maybe_unused]] static bool forEachInZip(
        const std::filesystem::path &p,
        std::function<bool(const std::filesystem::path&)> filterFunc,
        std::function<bool(const std::filesystem::path&, FileReader &)> op
    );

    static void forEachInZipOrRaw(
        const std::filesystem::path &p,
        std::function<bool(const std::filesystem::path&)> filterFunc,
        std::function<bool(const std::filesystem::path&, FileReader &)> op
    );

    static bool forRaw(
        const std::filesystem::path &p,
        std::function<bool(FileReader &)> op
    );

    static bool cmpPathExt(const std::filesystem::path &p, const std::string &ext);
};
