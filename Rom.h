#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

typedef uint32_t agbptr_t;

namespace agbplay {
    class Rom {
        public:
            Rom(std::string filePath);
            ~Rom();

            void Seek(long pos);
            void SeekAGBPtr(agbptr_t ptr);
            agbptr_t PosToAGBPtr(long pos);
            long AGBPtrToPos(agbptr_t ptr);

            int8_t ReadInt8();
            uint8_t ReadUInt8();
            int16_t ReadInt16();
            uint16_t ReadUInt16();
            int32_t ReadInt32();
            uint32_t ReadUInt32();
            long ReadAGBPtrToPos();
            std::string ReadString(size_t limit);
            void *GetPtr();

            size_t Size();
            bool ValidPointer(agbptr_t ptr);

        private:
            void checkBounds(long pos, size_t typesz);
            void verify();

            std::vector<uint8_t> data;
            long pos;   // = 0 for ROM start
    };
}
