#include <thread>
#include <chrono>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <cassert>
#include <cstring>

#if __has_include(<pa_win_wasapi.h>)
#include <pa_win_wasapi.h>
#endif

#include "PlaybackEngine.h"
#include "Xcept.h"
#include "Debug.h"
#include "Util.h"
#include "ConfigManager.h"

/*
 * PlaybackEngine data
 */

// first portaudio hostapi has highest priority, last hostapi has lowest
// if none are available, the default one is selected.
// they are also the ones which are known to work
const std::vector<PaHostApiTypeId> PlaybackEngine::hostApiPriority = {
    // Unix
    paJACK,
    paALSA,
    // Windows
    paWASAPI,
    paMME,
    // Mac OS
    paCoreAudio,
    paSoundManager,
};

/*
 * public PlaybackEngine
 */

PlaybackEngine::PlaybackEngine(size_t initSongPos)
    : mutedTracks(ConfigManager::Instance().GetCfg().GetTrackLimit())
{
    initContext();
    ctx->InitSong(initSongPos);
    setupLoudnessCalcs();
    portaudioOpen();
}

PlaybackEngine::~PlaybackEngine() 
{
    // stop and deallocate player thread if required
    Stop();
    portaudioClose();
}

void PlaybackEngine::LoadSong(size_t songPos)
{
    bool play = playerState == State::PLAYING;
    Stop();
    ctx->InitSong(songPos);
    setupLoudnessCalcs();
    updatePlaybackState(true);

    if (play)
        Play();
}

void PlaybackEngine::Play()
{
    switch (playerState) {
    case State::RESTART:
        // --> handled by worker
        break;
    case State::PLAYING:
        // restart song if player is running
        playerState = State::RESTART;
        break;
    case State::PAUSED:
        // continue paused playback
        playerState = State::PLAYING;
        break;
    case State::TERMINATED:
        // thread needs to be deleted before restarting
        Stop();
        Play();
        break;
    case State::SHUTDOWN:
        // --> handled by worker
        break;
    case State::THREAD_DELETED:
        playerState = State::PLAYING;
        playerThread = std::make_unique<std::thread>(&PlaybackEngine::threadWorker, this);
#ifdef __linux__
        pthread_setname_np(playerThread->native_handle(), "mixer thread");
#endif
        // start thread and play back song
        break;
    }
}

void PlaybackEngine::Pause()
{
    switch (playerState) {
        case State::RESTART:
            // --> handled by worker
            break;
        case State::PLAYING:
            playerState = State::PAUSED;
            break;
        case State::PAUSED:
            playerState = State::PLAYING;
            break;
        case State::TERMINATED:
            // ingore this
            break;
        case State::SHUTDOWN:
            // --> handled by worker
            break;
        case State::THREAD_DELETED:
            Play();
            break;
    }
}

void PlaybackEngine::Stop()
{
    switch (playerState) {
        case State::RESTART:
            // wait until player has initialized and quit then
            while (playerState != State::PLAYING) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            Stop();
            break;
        case State::PLAYING:
            playerState = State::SHUTDOWN;
            Stop();
            break;
        case State::PAUSED:
            playerState = State::SHUTDOWN;
            Stop();
            break;
        case State::TERMINATED:
        case State::SHUTDOWN:
            playerThread->join();
            playerThread.reset();
            playerState = State::THREAD_DELETED;
            break;            
        case State::THREAD_DELETED:
            // ignore this
            break;
    }
}

void PlaybackEngine::SpeedDouble()
{
    speedFactor <<= 1;
    if (speedFactor > 1024)
        speedFactor = 1024;
    ctx->reader.SetSpeedFactor(float(speedFactor) / 64.0f);
}

void PlaybackEngine::SpeedHalve()
{
    speedFactor >>= 1;
    if (speedFactor < 1)
        speedFactor = 1;
    ctx->reader.SetSpeedFactor(float(speedFactor) / 64.0f);
}

bool PlaybackEngine::IsPlaying()
{
    return playerState != State::THREAD_DELETED && playerState != State::TERMINATED;
}

bool PlaybackEngine::IsPaused() const
{
    return playerState == State::PAUSED;
}

void PlaybackEngine::ToggleMute(size_t index)
{
    mutedTracks[index] = !mutedTracks[index];
}

void PlaybackEngine::Mute(size_t index, bool mute)
{
    mutedTracks[index] = mute;
}

SongInfo PlaybackEngine::GetSongInfo() const
{
    SongInfo result;
    result.songHeaderPos = ctx->seq.GetSongHeaderPos();
    result.voiceTablePos = ctx->seq.GetSoundBankPos();
    result.reverb = ctx->seq.GetReverb();
    result.priority = ctx->seq.GetPriority();
    return result;
}

const PlaybackSongState &PlaybackEngine::GetPlaybackSongState() const
{
    return songState;
}

/*
 * private PlaybackEngine
 */

void PlaybackEngine::initContext()
{
    const auto &cm = ConfigManager::Instance();
    const auto &cfg = cm.GetCfg();

    const MP2KSoundMode soundMode{
        cfg.GetPCMVol(), cfg.GetEngineRev(), cfg.GetEngineFreq()
    };

    const AgbplayMixingOptions mixingOptions{
        .resamplerTypeNormal = cfg.GetResType(),
        .resamplerTypeFixed = cfg.GetResTypeFixed(),
        .reverbType = cfg.GetRevType(),
        .cgbPolyphony = cm.GetCgbPolyphony(),
        .dmaBufferLen = cfg.GetRevBufSize(),
        .trackLimit = cfg.GetTrackLimit(),
        .maxLoops = cm.GetMaxLoopsPlaylist(),
        .padSilenceSecondsStart = cm.GetPadSecondsStart(),
        .padSilenceSecondsEnd = cm.GetPadSecondsEnd(),
        .accurateCh3Quantization = cfg.GetAccurateCh3Quantization(),
        .accurateCh3Volume = cfg.GetAccurateCh3Volume(),
        .emulateCgbSustainBug = cfg.GetSimulateCGBSustainBug(),
    };

    ctx = std::make_unique<PlayerContext>(
        soundMode,
        mixingOptions
    );
}

void PlaybackEngine::threadWorker()
{
    size_t samplesPerBuffer = ctx->mixer.GetSamplesPerBuffer();
    std::vector<sample> silence(samplesPerBuffer, sample{0.0f, 0.0f});
    std::vector<sample> masterAudio(samplesPerBuffer, sample{0.0f, 0.0f});
    std::vector<std::vector<sample>> trackAudio;
    try {
        while (playerState != State::SHUTDOWN) {
            switch (playerState) {
            case State::RESTART:
                ctx->InitSong(ctx->seq.GetSongHeaderPos());
                playerState = State::PLAYING;
                [[fallthrough]];
            case State::PLAYING:
                {
                    // clear high level mixing buffer
                    fill(masterAudio.begin(), masterAudio.end(), sample{0.0f, 0.0f});
                    // render audio buffers for tracks
                    ctx->Process(trackAudio);
                    for (size_t i = 0; i < trackAudio.size(); i++) {
                        assert(trackAudio[i].size() == masterAudio.size());

                        bool muteThis = mutedTracks[i];
                        ctx->seq.tracks[i].muted = muteThis;
                        trackLoudness[i].CalcLoudness(trackAudio[i].data(), samplesPerBuffer);
                        if (muteThis)
                            continue;

                        for (size_t j = 0; j < masterAudio.size(); j++) {
                            masterAudio[j].left += trackAudio[i][j].left;
                            masterAudio[j].right += trackAudio[i][j].right;
                        }
                    }

                    // blocking write to audio buffer
                    rBuf.Put(masterAudio.data(), masterAudio.size());
                    masterLoudness.CalcLoudness(masterAudio.data(), samplesPerBuffer);
                    updatePlaybackState();

                    if (ctx->HasEnded()) {
                        playerState = State::SHUTDOWN;
                        break;
                    }
                }
                break;
            case State::PAUSED:
                rBuf.Put(silence.data(), silence.size());
                break;
            default:
                throw Xcept("Internal PlaybackEngine error: %d", (int)playerState);
            }
        }
        // reset song state after it has finished
        ctx->InitSong(ctx->seq.GetSongHeaderPos());
    } catch (std::exception& e) {
        Debug::print("FATAL ERROR on streaming thread: %s", e.what());
    }
    masterLoudness.Reset();
    for (LoudnessCalculator& c : trackLoudness)
        c.Reset();
    // flush buffer
    rBuf.Clear();
    playerState = State::TERMINATED;
}

void PlaybackEngine::updatePlaybackState(bool reset)
{
    if (!reset) {
        if (playerState == State::THREAD_DELETED ||
                playerState == State::SHUTDOWN ||
                playerState == State::TERMINATED)
            return;
    }

    const size_t ntrks = ctx->seq.tracks.size();
    assert(ntrks == trackLoudness.size());
    std::array<std::pair<float, float>, MAX_TRACKS> vols;
    for (size_t i = 0; i < ntrks; i++)
        trackLoudness[i].GetLoudness(vols[i].first, vols[i].second);

    /* Code below should probably be atomic, but we generally do not care about changes to the variables
     * arriving late to the reading thread. */
    songState.activeChannels = static_cast<int>(ctx->sndChannels.size());
    if (reset)
        songState.maxChannels = 0;
    else
        songState.maxChannels = std::max(songState.maxChannels, songState.activeChannels);
    songState.tracks_used = ntrks;

    for (size_t i = 0; i < ntrks; i++) {
        const auto &trk_src = ctx->seq.tracks[i];
        auto &trk_dst = songState.tracks[i];

        trk_dst.trackPtr = static_cast<uint32_t>(trk_src.pos);
        trk_dst.isCalling = trk_src.reptCount > 0;
        trk_dst.isMuted = trk_src.muted;
        trk_dst.vol = trk_src.vol;
        trk_dst.mod = trk_src.mod;
        trk_dst.prog = trk_src.prog;
        trk_dst.pan = trk_src.pan;
        trk_dst.pitch = trk_src.pitch;
        trk_dst.envL = uint8_t(std::clamp<uint32_t>(uint32_t(vols[i].first * 768.f), 0, 255));
        trk_dst.envR = uint8_t(std::clamp<uint32_t>(uint32_t(vols[i].second * 768.f), 0, 255));
        trk_dst.delay = std::max<uint8_t>(0, static_cast<uint8_t>(trk_src.delay));
        trk_dst.activeNotes = trk_src.activeNotes;
    }

    masterLoudness.GetLoudness(songState.masterVolLeft, songState.masterVolRight);
}

int PlaybackEngine::audioCallback(const void *inputBuffer, void *outputBuffer, size_t framesPerBuffer,
        const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
    (void)inputBuffer;
    (void)timeInfo;
    (void)statusFlags;
    Ringbuffer *rBuf = (Ringbuffer *)userData;
    rBuf->Take((sample *)outputBuffer, framesPerBuffer);
    return 0;
}

void PlaybackEngine::setupLoudnessCalcs()
{
    trackLoudness.clear();
    for (size_t i = 0; i < ctx->seq.tracks.size(); i++)
        trackLoudness.emplace_back(5.0f);
}

void PlaybackEngine::portaudioOpen()
{
    // init host api
    std::vector<PaHostApiTypeId> hostApiPrioritiesWithFallback = hostApiPriority;
    const PaHostApiIndex defaultHostApiIndex = Pa_GetDefaultHostApi();
    if (defaultHostApiIndex < 0)
        throw Xcept("Pa_GetDefaultHostApi(): No host API avilable: %s", Pa_GetErrorText(defaultHostApiIndex));
    const PaHostApiInfo *defaultHostApiInfo = Pa_GetHostApiInfo(defaultHostApiIndex);
    if (defaultHostApiInfo == nullptr)
        throw Xcept("Pa_GetHostApiInfo(): failed with valid index");
    const auto f = std::find(hostApiPrioritiesWithFallback.begin(), hostApiPrioritiesWithFallback.end(), defaultHostApiInfo->type);
    if (f == hostApiPrioritiesWithFallback.end())
        hostApiPrioritiesWithFallback.push_back(defaultHostApiInfo->type);

    bool streamOpen = false;

    for (const auto apiType : hostApiPrioritiesWithFallback) {
        const PaHostApiIndex hostApiIndex = Pa_HostApiTypeIdToHostApiIndex(apiType);
        // prioritized host api available ?
        if (hostApiIndex < 0)
            continue;

        const PaHostApiInfo *apiInfo = Pa_GetHostApiInfo(hostApiIndex);
        if (apiInfo == nullptr)
            throw Xcept("Pa_GetHostApiInfo with valid index failed");
        const PaDeviceIndex deviceIndex = apiInfo->defaultOutputDevice;

        std::shared_ptr<void> hostApiSpecificStreamInfo;

#if __has_include(<pa_win_wasapi.h>)
        if (apiType == paWASAPI) {
            PaWasapiStreamInfo info;
            memset(&info, 0, sizeof(info));
            info.size = sizeof(info);
            info.hostApiType = paWASAPI;
            info.version = 1;
            info.flags = paWinWasapiAutoConvert;
            hostApiSpecificStreamInfo = std::make_shared<PaWasapiStreamInfo>(info);
        }
#endif

        const PaDeviceInfo *devInfo = Pa_GetDeviceInfo(deviceIndex);
        if (devInfo == nullptr)
            throw Xcept("Pa_GetDeviceInfo(): failed with valid index");

        PaStreamParameters outputStreamParameters;
        outputStreamParameters.device = deviceIndex;
        outputStreamParameters.channelCount = 2;    // stereo
        outputStreamParameters.sampleFormat = paFloat32;
        outputStreamParameters.suggestedLatency = devInfo->defaultLowOutputLatency;
        outputStreamParameters.hostApiSpecificStreamInfo = hostApiSpecificStreamInfo.get();

        const uint32_t rate = ctx->mixer.GetSampleRate();
        PaError err = Pa_OpenStream(&audioStream, nullptr, &outputStreamParameters, rate, paFramesPerBufferUnspecified, paNoFlag, audioCallback, (void *)&rBuf);
        if (err != paNoError) {
            Debug::print("Pa_OpenStream(): unable to open stream with host API %s: %s", apiInfo->name, Pa_GetErrorText(err));
            continue;
        }

        err = Pa_StartStream(audioStream);
        if (err != paNoError) {
            Debug::print("Pa_StartStream(): unable to start stream for Host API %s: %s", apiInfo->name, Pa_GetErrorText(err));
            err = Pa_CloseStream(audioStream);
            if (err != paNoError)
                Debug::print("Pa_CloseStream(): unable to close fail-started stream for Host API %s: %s", apiInfo->name, Pa_GetErrorText(err));
            continue;
        }

        streamOpen = true;
        break;
    }

    if (!streamOpen)
        throw Xcept("Unable to initialize sound output: Host API could not be initialized");
}

void PlaybackEngine::portaudioClose()
{
    PaError err;
    if ((err = Pa_StopStream(audioStream)) != paNoError) {
        Debug::print("Pa_StopStream: %s", Pa_GetErrorText(err));
    }
    if ((err = Pa_CloseStream(audioStream)) != paNoError) {
        Debug::print("Pa_CloseStream: %s", Pa_GetErrorText(err));
    }
}
