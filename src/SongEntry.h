#pragma once

#include <cstdint>
#include <string>

namespace agbplay
{
    class SongEntry 
    {
        public:
            SongEntry(std::string name, uint16_t uid);
            ~SongEntry();

            uint16_t GetUID();
            std::string name;
        private:
            uint16_t uid;
    };
}
