#pragma once

#include <vector>
#include <string>

namespace agbplay 
{
    class FileContainer
    {
        public:
            FileContainer(std::string filePath);
            ~FileContainer();
            std::vector<uint8_t> data;
    };
}
