#include <sndfile.h>
#include <filesystem>
#include <boost/algorithm/string/replace.hpp>
#include <chrono>
#include <climits>

#include "SoundExporter.h"
#include "Util.h"
#include "Xcept.h"
#include "Constants.h"
#include "Debug.h"
#include "ConfigManager.h"
#include "PlayerContext.h"

/*
 * public SoundExporter
 */

SoundExporter::SoundExporter(SongTable& songTable, bool benchmarkOnly, bool seperate)
: songTable(songTable), benchmarkOnly(benchmarkOnly), seperate(seperate)
{
}

void SoundExporter::Export(const std::vector<SongEntry>& entries)
{
    std::filesystem::path dir = ConfigManager::Instance().GetWavOutputDir();
    if (std::filesystem::exists(dir)) {
        if (!std::filesystem::is_directory(dir)) {
            throw Xcept("Output directory exists but isn't a dir");
        }
    }
    else if (!std::filesystem::create_directories(dir)) {
        throw Xcept("Creating output directory failed");
    }

    size_t totalBlocksRendered = 0;

    auto startTime = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < entries.size(); i++)
    {
        std::string fname = entries[i].name;
        boost::replace_all(fname, "/", "_");
        Debug::print("%3d %% - Rendering to file: \"%s\"", (i + 1) * 100 / entries.size(), fname.c_str());
        char fileName[512];
        snprintf(fileName, sizeof(fileName), "%s/%03zu - %s", dir.c_str(), i + 1, fname.c_str());
        size_t rblocks = exportSong(fileName, entries[i].GetUID());
        totalBlocksRendered += rblocks;
    }

    auto endTime = std::chrono::high_resolution_clock::now();

    if (std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count() == 0) {
        Debug::print("Successfully wrote %zu files", entries.size());
    } else {
        size_t blocksPerSecond = totalBlocksRendered / (size_t)std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count();
        Debug::print("Successfully wrote %zu files at %zu blocks per second", entries.size(), blocksPerSecond);
    }
}

/*
 * private SoundExporter
 */

size_t SoundExporter::exportSong(const std::filesystem::path& fileName, uint16_t uid)
{
    // setup our generators
    GameConfig& cfg = ConfigManager::Instance().GetCfg();

    
    PlayerContext ctx(
            1, 
            cfg.GetTrackLimit(),
            EnginePars(cfg.GetPCMVol(), cfg.GetEngineRev(), cfg.GetEngineFreq())
            );
    ctx.InitSong(songTable.GetPosOfSong(uid));
    size_t blocksRendered = 0;
    size_t nBlocks = ctx.mixer.GetSamplesPerBuffer();
    size_t nTracks = ctx.seq.tracks.size();
    std::vector<std::vector<sample>> trackAudio;

    if (!benchmarkOnly) 
    {
        /* save each track to a separate file */
        if (seperate)
        {
            std::vector<SNDFILE *> ofiles(nTracks, nullptr);
            std::vector<SF_INFO> oinfos(nTracks);

            for (size_t i = 0; i < nTracks; i++)
            {
                memset(&oinfos[i], 0, sizeof(oinfos[i]));
                oinfos[i].samplerate = STREAM_SAMPLERATE;
                oinfos[i].channels = 2; // stereo
                oinfos[i].format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
                char outName[PATH_MAX];
                snprintf(outName, sizeof(outName), "%s.%02zu.wav", fileName.c_str(), i);
                ofiles[i] = sf_open(outName, SFM_WRITE, &oinfos[i]);
                if (ofiles[i] == NULL)
                    Debug::print("Error: %s", sf_strerror(NULL));
            }

            while (true)
            {
                ctx.reader.Process();
                ctx.mixer.Process(trackAudio);
                if (ctx.HasEnded())
                    break;

                assert(trackAudio.size() == nTracks);

                for (size_t i = 0; i < nTracks; i++) 
                {
                    // do not write to invalid files
                    if (ofiles[i] == NULL)
                        continue;
                    sf_count_t processed = 0;
                    do {
                        processed += sf_writef_float(ofiles[i], &trackAudio[i][processed].left, sf_count_t(nBlocks) - processed);
                    } while (processed < sf_count_t(nBlocks));
                }
                blocksRendered += nBlocks;
            }

            for (SNDFILE *& i : ofiles)
            {
                int err = sf_close(i);
                if (err != 0)
                    Debug::print("Error: %s", sf_error_number(err));
            }
        }
        else
        {
            SF_INFO oinfo;
            memset(&oinfo, 0, sizeof(oinfo));
            oinfo.samplerate = STREAM_SAMPLERATE;
            oinfo.channels = 2; // sterep
            oinfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
            SNDFILE *ofile = sf_open((fileName.string() + ".wav").c_str(), SFM_WRITE, &oinfo);
            if (ofile == NULL) {
                Debug::print("Error: %s", sf_strerror(NULL));
                return 0;
            }
            // do rendering and write
            std::vector<sample> renderedData(nBlocks);

            while (true) 
            {
                ctx.reader.Process();
                ctx.mixer.Process(trackAudio);
                if (ctx.HasEnded())
                    break;
                // mix streams to one master
                assert(trackAudio.size() == nTracks);
                // clear mixing buffer
                fill(renderedData.begin(), renderedData.end(), sample{0.0f, 0.0f});
                // mix all tracks to buffer
                for (std::vector<sample>& b : trackAudio)
                {
                    assert(b.size() == renderedData.size());
                    for (size_t i = 0; i < b.size(); i++) {
                        renderedData[i].left  += b[i].left;
                        renderedData[i].right += b[i].right;
                    }
                }
                sf_count_t processed = 0;
                do {
                    processed += sf_writef_float(ofile, &renderedData[processed].left, sf_count_t(nBlocks) - processed);
                } while (processed < sf_count_t(nBlocks));
                blocksRendered += nBlocks;
            }

            int err;
            if ((err = sf_close(ofile)) != 0)
                Debug::print("Error: %s", sf_error_number(err));
        }
    } 
    // if benchmark only
    else {
        while (true)
        {
            ctx.reader.Process();
            ctx.mixer.Process(trackAudio);
            blocksRendered += nBlocks;
            if (ctx.HasEnded())
                break;
        }
    }
    return blocksRendered;
}
