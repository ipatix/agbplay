#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>
#include <memory>

#include "AgbTypes.h"
#include "Xcept.h"

class Rom {
public:
    Rom(const std::filesystem::path& filePath);
    Rom(const Rom&) = delete;
    Rom& operator=(const Rom&) = delete;
    static void CreateInstance(const std::filesystem::path& filePath);
    static Rom& Instance() {
        return *global_instance;
    }

    const uint8_t& operator[](size_t pos) const {
        return data[pos];
    }

    int8_t ReadS8(size_t pos) const {
        return static_cast<int8_t>(data.at(pos));
    }

    uint8_t ReadU8(size_t pos) const {
        return data.at(pos);
    }

    int16_t ReadS16(size_t pos) const {
        return static_cast<int16_t>(ReadU16(pos));
    }

    uint16_t ReadU16(size_t pos) const {
        uint32_t retval = data.at(pos + 1);
        retval <<= 8;
        retval |= data[pos];
        return static_cast<uint16_t>(retval);
    }

    int32_t ReadS32(size_t pos) const {
        return static_cast<int32_t>(ReadU32(pos));
    }

    uint32_t ReadU32(size_t pos) const {
        uint32_t retval = data.at(pos + 3);
        retval <<= 8;
        retval |= data[pos + 2];
        retval <<= 8;
        retval |= data[pos + 1];
        retval <<= 8;
        retval |= data[pos + 0];
        return retval;
    }

    size_t ReadAgbPtrToPos(size_t pos) const {
        uint32_t ptr = ReadU32(pos);
        if (!ValidPointer(ptr))
            throw Xcept("Cannot parse pointer at [%08zX]=%08X", pos, ptr);
        return ptr - AGB_MAP_ROM;
    }

    const void *GetPtr(size_t pos) const {
        return &data[pos];
    }

    size_t Size() const {
        return data.size();
    }

    bool ValidPointer(uint32_t ptr) const {
        if (ptr - AGB_MAP_ROM >= data.size())
            return false;
        if (ptr - AGB_MAP_ROM + 1 >= data.size())
            return false;
        return true;
    }

    bool ValidRange(size_t pos, size_t len) const {
        if (pos + len >= data.size())
            return false;
        return true;
    }

    std::string ReadString(size_t pos, size_t limit) const;
    std::string GetROMCode() const;

private:
    void verify();
    void loadFile(const std::filesystem::path& filePath);

    std::vector<uint8_t> data;

    static std::unique_ptr<Rom> global_instance;
};
