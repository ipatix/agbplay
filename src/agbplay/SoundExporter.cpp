#include "SoundExporter.hpp"

#include "Constants.hpp"
#include "Debug.hpp"
#include "MP2KContext.hpp"
#include "OS.hpp"
#include "Profile.hpp"
#include "Util.hpp"
#include "Xcept.hpp"

#include <atomic>
#include <boost/algorithm/string/replace.hpp>
#include <chrono>
#include <climits>
#include <cmath>
#include <codecvt>
#include <filesystem>
#include <mutex>
#include <sndfile.h>
#include <thread>

/*
 * public SoundExporter
 */

SoundExporter::SoundExporter(
    const std::filesystem::path &directory,
    uint32_t sampleRate,
    const Profile &profile,
    bool benchmarkOnly,
    bool seperate
) :
    directory(directory), sampleRate(sampleRate), profile(profile), benchmarkOnly(benchmarkOnly), seperate(seperate)
{
}

void SoundExporter::Export()
{
    if (!benchmarkOnly) {
        Debug::print("Starting export to directory: {}", directory.string());
        /* create directories for file export */
        if (std::filesystem::exists(directory)) {
            if (!std::filesystem::is_directory(directory)) {
                throw Xcept("Output directory exists but isn't a directory");
            }
        } else if (!std::filesystem::create_directories(directory)) {
            throw Xcept("Creating output directory failed");
        }
    }

    /* setup export thread worker function */
    std::atomic<size_t> currentSong = 0;
    std::atomic<size_t> totalSamplesRendered = 0;

    std::function<void(void)> threadFunc = [&]() {
        OS::LowerThreadPriority();
        while (true) {
            size_t i = currentSong++;    // atomic ++
            if (i >= profile.playlist.size())
                return;

            /* name's in profile are utf8 encoded */
            std::string name = profile.playlist.at(i).name;
            ReplaceIllegalPathCharacters(name, '_');
            Debug::print("{:3}% - Rendering to file: \"{}\"", (i + 1) * 100 / profile.playlist.size(), name);
            std::u8string u8name(reinterpret_cast<const char8_t *>(name.c_str()));
            std::filesystem::path filePath = directory;
            filePath /= fmt::format("{:03d} - ", i + 1);
            filePath += u8name;
            totalSamplesRendered += exportSong(filePath, profile.playlist.at(i).id);
        }
    };

    /* run the actual export threads */
    auto startTime = std::chrono::high_resolution_clock::now();

    size_t numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0)
        numThreads = 1;
    std::vector<std::thread> workers;
    for (size_t i = 0; i < numThreads; i++)
        workers.emplace_back(threadFunc);
    for (auto &w : workers)
        w.join();
    workers.clear();

    auto endTime = std::chrono::high_resolution_clock::now();

    /* report finished progress */
    if (std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count() == 0) {
        Debug::print("Successfully wrote {} files", profile.playlist.size());
    } else {
        const uint64_t secondsTotal =
            static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count());
        const uint64_t microSecondsTotal =
            static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count());
        const uint64_t samplesPerSecond = totalSamplesRendered * 1000000 / microSecondsTotal;
        Debug::print(
            "Successfully wrote {} files at {} samples per second ({} seconds total)",
            profile.playlist.size(),
            samplesPerSecond,
            secondsTotal
        );
    }
}

/*
 * private SoundExporter
 */

void SoundExporter::writeSilence(SNDFILE *ofile, double seconds)
{
    if (seconds <= 0.0)
        return;
    size_t samples = static_cast<size_t>(std::round(sampleRate * seconds));
    std::vector<sample> silence(samples, {0.0f, 0.0f});
    sf_writef_float(ofile, reinterpret_cast<float *>(silence.data()), static_cast<sf_count_t>(silence.size()));
}

size_t SoundExporter::exportSong(const std::filesystem::path &filePath, uint16_t uid)
{
    MP2KContext ctx(
        sampleRate,
        Rom::Instance(),
        profile.mp2kSoundModePlayback,
        profile.agbplaySoundMode,
        profile.songTableInfoPlayback,
        profile.playerTablePlayback
    );

    ctx.m4aSongNumStart(uid);

    const uint8_t playerIdx = ctx.m4aSongNumPlayerGet(uid);
    size_t samplesRendered = 0;
    size_t samplesPerBuffer = ctx.mixer.GetSamplesPerBuffer();
    size_t nTracks = ctx.players.at(playerIdx).tracksUsed;
    const double padSecondsStart = profile.agbplaySoundMode.padSilenceSecondsStart;
    const double padSecondsEnd = profile.agbplaySoundMode.padSilenceSecondsEnd;

    if (!benchmarkOnly) {
        /* save each track to a separate file */
        if (seperate) {
            std::vector<SNDFILE *> ofiles(nTracks, nullptr);
            std::vector<SF_INFO> oinfos(nTracks);

            for (size_t i = 0; i < nTracks; i++) {
                memset(&oinfos[i], 0, sizeof(oinfos[i]));
                oinfos[i].samplerate = sampleRate;
                oinfos[i].channels = 2;    // stereo
                oinfos[i].format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
                std::filesystem::path finalFilePath = filePath;
                finalFilePath += fmt::format(".{:02d}.wav", i);
#ifdef _WIN32
                ofiles[i] = sf_wchar_open(finalFilePath.wstring().c_str(), SFM_WRITE, &oinfos[i]);
#else
                ofiles[i] = sf_open(finalFilePath.string().c_str(), SFM_WRITE, &oinfos[i]);
#endif
                if (ofiles[i] == NULL)
                    Debug::print("Error: {}", sf_strerror(NULL));
            }

            while (true) {
                ctx.m4aSoundMain();
                if (ctx.HasEnded())
                    break;

                assert(ctx.players.at(playerIdx).tracks.size() == nTracks);

                for (size_t i = 0; i < nTracks; i++) {
                    // do not write to invalid files
                    if (ofiles[i] == NULL)
                        continue;
                    sf_count_t processed = 0;
                    do {
                        processed += sf_writef_float(
                            ofiles[i],
                            &ctx.players.at(playerIdx).tracks.at(i).audioBuffer[processed].left,
                            sf_count_t(samplesPerBuffer) - processed
                        );
                    } while (processed < sf_count_t(samplesPerBuffer));
                }
                samplesRendered += samplesPerBuffer;
            }

            for (SNDFILE *&i : ofiles) {
                int err = sf_close(i);
                if (err != 0)
                    Debug::print("Error: {}", sf_error_number(err));
            }
        } else {
            SF_INFO oinfo;
            memset(&oinfo, 0, sizeof(oinfo));
            oinfo.samplerate = sampleRate;
            oinfo.channels = 2;    // sterep
            oinfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
            std::filesystem::path finalFilePath = filePath;
            finalFilePath += fmt::format(".wav");
#ifdef _WIN32
            SNDFILE *ofile = sf_wchar_open(finalFilePath.wstring().c_str(), SFM_WRITE, &oinfo);
#else
            SNDFILE *ofile = sf_open(finalFilePath.string().c_str(), SFM_WRITE, &oinfo);
#endif
            if (ofile == NULL) {
                Debug::print("Error: {}", sf_strerror(NULL));
                return 0;
            }

            writeSilence(ofile, padSecondsStart);

            while (true) {
                ctx.m4aSoundMain();
                if (ctx.HasEnded())
                    break;

                sf_count_t processed = 0;
                do {
                    processed += sf_writef_float(
                        ofile, &ctx.masterAudioBuffer[processed].left, sf_count_t(samplesPerBuffer) - processed
                    );
                } while (processed < sf_count_t(samplesPerBuffer));
                samplesRendered += samplesPerBuffer;
            }

            writeSilence(ofile, padSecondsEnd);

            int err;
            if ((err = sf_close(ofile)) != 0)
                Debug::print("Error: {}", sf_error_number(err));
        }
    }
    // if benchmark only
    else {
        while (true) {
            ctx.m4aSoundMain();
            samplesRendered += samplesPerBuffer;
            if (ctx.HasEnded())
                break;
        }
    }
    return samplesRendered;
}
