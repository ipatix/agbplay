#pragma once

#include <vector>
#include <string>

#include "GameConfig.h"

namespace agbplay
{
    class ConfigManager
    {
        public:
            ~ConfigManager();

            static ConfigManager& Instance();

            GameConfig& GetCfg();
            void SetGameCode(const std::string& gameCode);
            void Save();
        private:
            ConfigManager();
            std::vector<GameConfig> configs;
            std::string configPath;
            GameConfig *curCfg = nullptr;
    };
}
