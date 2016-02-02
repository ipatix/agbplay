#pragma once

namespace agbplay
{
    class SongEntry 
    {
        public:
            SongEntry(std::string name, uint16_t uid);
            ~SongEntry();

            uint16_t GetUID();
        private:
            std::string name;
            uint16_t uid;
    };
}
