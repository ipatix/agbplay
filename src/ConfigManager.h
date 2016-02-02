#pragma once

#include <vector>
#include <string>

#include "GameConfig.h"

namespace agbplay
{
    class ConfigManager
    {
        public:
            ConfigManager(std::string configFile);
            ~ConfigManager();
        private:
            std::vector<GameConfig> configs;
            std::string configFile;
    };
}
