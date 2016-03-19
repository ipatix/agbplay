#include <sndfile.h>
#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/filesystem.hpp>
#undef BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/algorithm/string/replace.hpp>
#include <boost/thread/mutex.hpp>
#include <chrono>

#include "SoundExporter.h"
#include "Util.h"
#include "MyException.h"
#include "Constants.h"

using namespace agbplay;
using namespace std;

/*
 * public SoundExporter
 */

SoundExporter::SoundExporter(ConsoleGUI& _con, SoundData& _sd, GameConfig& _cfg, Rom& _rom, bool _benchmarkOnly)
    : con(_con), cfg(_cfg), sd(_sd), rom(_rom)
{
    benchmarkOnly = _benchmarkOnly;
}

SoundExporter::~SoundExporter()
{
}

void SoundExporter::Export(string outputDir, vector<SongEntry>& entries, vector<bool>& ticked)
{
    if (entries.size() != ticked.size())
        throw MyException("SoundExporter: input vectors do not match");
    vector<SongEntry> tEnts;
    for (size_t i = 0; i < entries.size(); i++) {
        if (!ticked[i])
            continue;
        tEnts.push_back(entries[i]);
    }


    boost::filesystem::path dir(outputDir);
    if (boost::filesystem::exists(dir)) {
        if (!boost::filesystem::is_directory(dir)) {
            throw MyException("Output directory exists but isn't a dir");
        }
    }
    else if (!boost::filesystem::create_directory(dir)) {
        throw MyException("Creating output directory failed");
    }

    size_t totalBlocksRendered = 0;

    auto startTime = chrono::high_resolution_clock::now();

#pragma omp parallel for// ordered schedule(dynamic)
    for (size_t i = 0; i < tEnts.size(); i++)
    {
        string fname = tEnts[i].name;
        boost::replace_all(fname, "/", "_");
        uilock.lock();
        con.WriteLn(FormatString("%3d%% - Rendering to file: \"%s\"", (i + 1) * 100 / tEnts.size(), fname));
        uilock.unlock();
        size_t rblocks = exportSong(FormatString("%s/%d - %s.wav", outputDir, i + 1, fname), tEnts[i].GetUID());
#pragma omp atomic
        totalBlocksRendered += rblocks;
    }

    auto endTime = chrono::high_resolution_clock::now();

    if (chrono::duration_cast<chrono::seconds>(endTime - startTime).count() == 0) {
        con.WriteLn(FormatString("Successfully wrote %d files", tEnts.size()));
    } else {
        con.WriteLn(FormatString("Successfully wrote %d files at %d blocks per second", 
                    tEnts.size(), 
                    int(totalBlocksRendered / (size_t)chrono::duration_cast<chrono::seconds>(endTime - startTime).count())));
    }
}

/*
 * private SoundExporter
 */

size_t SoundExporter::exportSong(string fileName, uint16_t uid)
{
    // setup our generators
    Sequence seq(sd.sTable->GetPosOfSong(uid), cfg.GetTrackLimit(), rom);
    StreamGenerator sg(seq, EnginePars(cfg.GetPCMVol(), cfg.GetEngineRev(), cfg.GetEngineFreq()), 1, 1.0f);
    size_t blocksRendered = 0;
    uint32_t nBlocks = sg.GetBufferUnitCount();
    // libsndfile setup
    if (!benchmarkOnly) {
        SF_INFO oinfo;
        memset(&oinfo, 0, sizeof(oinfo));
        oinfo.samplerate = STREAM_SAMPLERATE;
        oinfo.channels = N_CHANNELS;
        oinfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
        SNDFILE *ofile = sf_open(fileName.c_str(), SFM_WRITE, &oinfo);
        if (ofile == NULL) {
            uilock.lock();
            con.WriteLn(FormatString("Error: %s", sf_strerror(NULL)));
            uilock.unlock();
            return 0;
        }
        // do rendering and write
        float *renderedData;

        while ((renderedData = sg.ProcessAndGetAudio()) != nullptr) {
            sf_count_t processed = 0;
            do {
                processed += sf_write_float(ofile, renderedData + processed, (nBlocks * N_CHANNELS) - processed);
            } while (processed < (nBlocks * N_CHANNELS));
            blocksRendered += nBlocks;
        }

        int err;
        if ((err = sf_close(ofile)) != 0) {
            uilock.lock();
            con.WriteLn(FormatString("Error: %s", sf_error_number(err)));
            uilock.unlock();
        }
    } 
    // if benchmark only
    else {
        while (sg.ProcessAndGetAudio())
            blocksRendered += nBlocks;
    }
    return blocksRendered;
}
