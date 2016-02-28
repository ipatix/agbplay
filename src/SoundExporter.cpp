#include <sndfile.h>
#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "SoundExporter.h"
#include "Util.h"
#include "MyException.h"
#include "Constants.h"

using namespace agbplay;
using namespace std;

/*
 * public SoundExporter
 */

SoundExporter::SoundExporter(ConsoleGUI& _con, SoundData& _sd, GameConfig& _cfg, Rom& _rom)
    : con(_con), cfg(_cfg), sd(_sd), rom(_rom)
{
}

SoundExporter::~SoundExporter()
{
}

void SoundExporter::Export(string outputDir, vector<SongEntry>& entries, vector<bool>& ticked)
{
    if (entries.size() != ticked.size())
        throw MyException("SoundExporter: input vectors do not match");
    size_t count = 0, total = 0, index = 0;
    for (bool t : ticked)
        if (t) total++;

    boost::filesystem::path dir(outputDir);
    if (boost::filesystem::exists(dir)) {
        if (!boost::filesystem::is_directory(dir)) {
            throw MyException("Output directory exists but isn't a dir");
        }
    }
    else if (!boost::filesystem::create_directory(dir)) {
        throw MyException("Creating output directory failed");
    }

    for (SongEntry& ent : entries)
    {
        size_t i = index++;
        if (!ticked[i])
            continue;
        string fname = ent.name;
        boost::replace_all(fname, "/", "_");
        con.WriteLn(FormatString("%3d%% - Rendering to file: \"%s\"", (count + 1) * 100 / total, fname));
        exportSong(FormatString("%s/%d - %s.wav", outputDir, count + 1, fname), ent.GetUID());
        count++;
    }
    con.WriteLn(FormatString("Successfully wrote %d files", count));
}

/*
 * private SoundExporter
 */

void SoundExporter::exportSong(string fileName, uint16_t uid)
{
    // setup our generators
    Sequence seq(sd.sTable->GetPosOfSong(uid), cfg.GetTrackLimit(), rom);
    StreamGenerator sg(seq, EnginePars(cfg.GetPCMVol(), cfg.GetEngineRev(), cfg.GetEngineFreq()), 1, 1.0f);
    // libsndfile setup
    SF_INFO oinfo = {
        .samplerate = STREAM_SAMPLERATE,
        .channels = N_CHANNELS,
        .format = SF_FORMAT_WAV | SF_FORMAT_FLOAT,
    };
    SNDFILE *ofile = sf_open(fileName.c_str(), SFM_WRITE, &oinfo);
    if (ofile == NULL) {
        con.WriteLn(FormatString("Error: %s", sf_strerror(NULL)));
        return;
    }
    // do rendering and write
    uint32_t nBlocks = sg.GetBufferUnitCount();
    float *renderedData;

    while ((renderedData = sg.ProcessAndGetAudio()) != nullptr) {
        sf_count_t processed = 0;
        do {
            processed += sf_write_float(ofile, renderedData + processed, (nBlocks * N_CHANNELS) - processed);
        } while (processed < (nBlocks * N_CHANNELS));
    }

    int err;
    if ((err = sf_close(ofile)) != 0) {
        con.WriteLn(FormatString("Error: %s", sf_error_number(err)));
    }
}
