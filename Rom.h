#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "FileContainer.h"

typedef uint32_t agbptr_t;

namespace agbplay {
    class Rom {
        public:
            Rom(FileContainer& fc);
            ~Rom();
            void Seek(long pos);
            void SeekAGBPtr(agbptr_t ptr);
            void RelSeek(int space);
            agbptr_t PosToAGBPtr(long pos);
            long AGBPtrToPos(agbptr_t ptr);
            int8_t ReadInt8();
            uint8_t ReadUInt8();
            int8_t PeekInt8();
            uint8_t PeekUInt8();
            int16_t ReadInt16();
            uint16_t ReadUInt16();
            int32_t ReadInt32();
            uint32_t ReadUInt32();
            long ReadAGBPtrToPos();
            std::string ReadString(size_t limit);
            void ReadData(void *dest, size_t bytes);
            void *GetPtr();
            size_t Size();
            bool ValidPointer(agbptr_t ptr);
        private:
            void checkBounds(long pos, size_t typesz);
            void verify();

            std::vector<uint8_t> *data;
            long pos;   // = 0 for ROM start
    };
}
