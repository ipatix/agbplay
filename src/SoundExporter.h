#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <mutex>

#include "StreamGenerator.h"
#include "SongEntry.h"
#include "GameConfig.h"
#include "ConsoleGUI.h"

namespace agbplay
{
    class SoundExporter
    {
        public:
            SoundExporter(ConsoleGUI& _con, SoundData& _sd, GameConfig& _cfg, Rom& _rom, bool _benchmarkOnly, bool seperate);
            ~SoundExporter();

            void Export(std::string outputDir, std::vector<SongEntry>& entries, std::vector<bool>& ticked);
        private:
            size_t exportSong(std::string fileName, uint16_t uid);

            ConsoleGUI& con;
            GameConfig& cfg;
            SoundData& sd;
            Rom& rom;
            std::mutex uilock;

            bool benchmarkOnly;
            bool seperate; // seperate tracks to multiple files
    };
}
