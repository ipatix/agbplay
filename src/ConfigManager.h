#pragma once

#include <vector>
#include <string>

#include "GameConfig.h"

namespace agbplay
{
    class ConfigManager
    {
        public:
            ConfigManager(const std::string& configPath);
            ~ConfigManager();

            GameConfig& GetConfig(std::string gameCode);
        private:
            std::vector<GameConfig> configs;
            std::string configPath;

    };
}
