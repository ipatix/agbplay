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

using namespace std;

/*
 * public SoundExporter
 */

SoundExporter::SoundExporter(ConsoleGUI& _con, SoundData& _sd, bool _benchmarkOnly, bool seperate)
: con(_con), sd(_sd)
{
    benchmarkOnly = _benchmarkOnly;
    this->seperate = seperate;
}

SoundExporter::~SoundExporter()
{
}

void SoundExporter::Export(const string& outputDir, vector<SongEntry>& entries, vector<bool>& ticked)
{
    if (entries.size() != ticked.size())
        throw Xcept("SoundExporter: input vectors do not match");
    vector<SongEntry> tEnts;
    for (size_t i = 0; i < entries.size(); i++) {
        if (!ticked[i])
            continue;
        tEnts.push_back(entries[i]);
    }


    filesystem::path dir(outputDir);
    if (filesystem::exists(dir)) {
        if (!filesystem::is_directory(dir)) {
            throw Xcept("Output directory exists but isn't a dir");
        }
    }
    else if (!filesystem::create_directory(dir)) {
        throw Xcept("Creating output directory failed");
    }

    size_t totalBlocksRendered = 0;

    auto startTime = chrono::high_resolution_clock::now();

    for (size_t i = 0; i < tEnts.size(); i++)
    {
        string fname = tEnts[i].name;
        boost::replace_all(fname, "/", "_");
        print_debug("%3d %% - Rendering to file: \"%s\"", (i + 1) * 100 / tEnts.size(), fname.c_str());
        char fileName[512];
        snprintf(fileName, sizeof(fileName), "%s/%03zu - %s", outputDir.c_str(), i + 1, fname.c_str());
        size_t rblocks = exportSong(fileName, tEnts[i].GetUID());
        totalBlocksRendered += rblocks;
    }

    auto endTime = chrono::high_resolution_clock::now();

    if (chrono::duration_cast<chrono::seconds>(endTime - startTime).count() == 0) {
        print_debug("Successfully wrote %zu files", tEnts.size());
    } else {
        print_debug("Successfully wrote %d files at %d blocks per second",
                    tEnts.size(), 
                    int(totalBlocksRendered / (size_t)chrono::duration_cast<chrono::seconds>(endTime - startTime).count()));
    }
}

/*
 * private SoundExporter
 */

size_t SoundExporter::exportSong(const string& fileName, uint16_t uid)
{
    // setup our generators
    GameConfig& cfg = ConfigManager::Instance().GetCfg();
    Sequence seq(sd.sTable->GetPosOfSong(uid), cfg.GetTrackLimit());
    StreamGenerator sg(seq, EnginePars(cfg.GetPCMVol(), cfg.GetEngineRev(), cfg.GetEngineFreq()), 1, 1.0f, cfg.GetRevType());
    size_t blocksRendered = 0;
    size_t nBlocks = sg.GetBufferUnitCount();
    size_t nTracks = seq.tracks.size();
    // libsndfile setup
    if (!benchmarkOnly) 
    {
        if (seperate)
        {
            vector<SNDFILE *> ofiles(nTracks, nullptr);
            vector<SF_INFO> oinfos(nTracks);

            for (size_t i = 0; i < nTracks; i++)
            {
                memset(&oinfos[i], 0, sizeof(oinfos[i]));
                oinfos[i].samplerate = STREAM_SAMPLERATE;
                oinfos[i].channels = N_CHANNELS;
                oinfos[i].format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
                char outName[PATH_MAX];
                snprintf(outName, sizeof(outName), "%s.%02zu.wav", fileName.c_str(), i);
                ofiles[i] = sf_open(outName, SFM_WRITE, &oinfos[i]);
                if (ofiles[i] == NULL)
                    print_debug("Error: %s", sf_strerror(NULL));
            }

            while (true)
            {
                vector<vector<float>>& rbuffers = sg.ProcessAndGetAudio();
                if (sg.HasStreamEnded())
                    break;

                assert(rbuffers.size() == nTracks);

                for (size_t i = 0; i < nTracks; i++) 
                {
                    // do not write to invalid files
                    if (ofiles[i] == NULL)
                        continue;
                    sf_count_t processed = 0;
                    do {
                        processed += sf_write_float(ofiles[i], rbuffers[i].data() + processed, sf_count_t(nBlocks * N_CHANNELS) - processed);
                    } while (processed < sf_count_t(nBlocks * N_CHANNELS));
                }
                blocksRendered += nBlocks;
            }

            for (SNDFILE *& i : ofiles)
            {
                int err = sf_close(i);
                if (err != 0)
                    print_debug("Error: %s", sf_error_number(err));
            }
        }
        else
        {
            SF_INFO oinfo;
            memset(&oinfo, 0, sizeof(oinfo));
            oinfo.samplerate = STREAM_SAMPLERATE;
            oinfo.channels = N_CHANNELS;
            oinfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
            SNDFILE *ofile = sf_open((fileName + ".wav").c_str(), SFM_WRITE, &oinfo);
            if (ofile == NULL) {
                print_debug("Error: %s", sf_strerror(NULL));
                return 0;
            }
            // do rendering and write
            vector<float> renderedData(nBlocks * N_CHANNELS);

            while (true) 
            {
                vector<vector<float>>& rbuffers = sg.ProcessAndGetAudio();
                if (sg.HasStreamEnded())
                    break;
                // mix streams to one master
                assert(rbuffers.size() == nTracks);
                // clear mixing buffer
                fill(renderedData.begin(), renderedData.end(), 0.0f);
                // mix all tracks to buffer
                for (vector<float>& b : rbuffers)
                {
                    assert(b.size() == renderedData.size());
                    for (size_t i = 0; i < b.size(); i++)
                        renderedData[i] += b[i];
                }
                sf_count_t processed = 0;
                do {
                    processed += sf_write_float(ofile, renderedData.data() + processed, sf_count_t(nBlocks * N_CHANNELS) - processed);
                } while (processed < sf_count_t(nBlocks * N_CHANNELS));
                blocksRendered += nBlocks;
            }

            int err;
            if ((err = sf_close(ofile)) != 0)
                print_debug("Error: %s", sf_error_number(err));
        }
    } 
    // if benchmark only
    else {
        while (true)
        {
            sg.ProcessAndGetAudio();
            blocksRendered += nBlocks;
            if (sg.HasStreamEnded())
                break;
        }
    }
    return blocksRendered;
}
