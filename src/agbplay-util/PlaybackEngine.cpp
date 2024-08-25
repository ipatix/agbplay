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
#if __has_include(<pa_jack.h>)
#include <pa_jack.h>
#endif

#include "PlaybackEngine.h"
#include "Xcept.h"
#include "Debug.h"
#include "Util.h"

/*
 * set JACK client name
 */

/* use a global object to run the function before Pa_Initialize() */

#if __has_include(<pa_jack.h>)
struct JackNameSetter {
    JackNameSetter() {
        PaJack_SetClientName("agbplay");
    }
} jackNameSetter;
#endif

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

PlaybackEngine::PlaybackEngine(const Profile &profile)
    : profile(profile)
{
    InitContext();
    ctx->m4aSongNumStart(0);
    ctx->m4aSongNumStop(0);
    portaudioOpen();
    playerThread = std::make_unique<std::thread>(&PlaybackEngine::threadWorker, this);
#ifdef __linux__
    pthread_setname_np(playerThread->native_handle(), "mixer thread");
#endif
}

PlaybackEngine::~PlaybackEngine() 
{
    // stop and deallocate player thread if required
    if (playerThread) {
        playerThreadQuit = true;
        playerThread->join();
    }

    portaudioClose();
}

void PlaybackEngine::LoadSong(uint16_t songIdx)
{
    trackMuted.reset();

    auto func = [this, songIdx]() {
        const uint8_t playerIdx = ctx->primaryPlayer;
        if (playerIdx >= ctx->players.size())
            return;

        ctx->m4aMPlayAllStop();
        ctx->m4aSongNumStart(songIdx);
        ctx->m4aMPlayStop(ctx->primaryPlayer);
        paused = false;
    };

    InvokeAsPlayer(func);
}

void PlaybackEngine::Play()
{
    auto func = [this]() {
        const uint8_t playerIdx = ctx->primaryPlayer;
        if (playerIdx >= ctx->players.size())
            return;

        if (paused) {
            paused = false;
        } else {
            MP2KPlayer &player = ctx->players.at(playerIdx);
            ctx->m4aMPlayStart(playerIdx, player.GetSongHeaderPos());
            for (size_t i = 0; i < std::min(player.tracks.size(), trackMuted.size()); i++)
                player.tracks.at(i).muted = trackMuted[i];
            hasEnded = false;
        }
    };

    InvokeAsPlayer(func);
}

bool PlaybackEngine::Pause()
{
    auto func = [this]() {
        const uint8_t playerIdx = ctx->primaryPlayer;
        if (playerIdx >= ctx->players.size())
            return;

        const bool playing = ctx->m4aMPlayIsPlaying(playerIdx);
        if (playing) {
            paused = !paused;
        } else {
            MP2KPlayer &player = ctx->players.at(playerIdx);
            ctx->m4aMPlayStart(playerIdx, ctx->players.at(playerIdx).GetSongHeaderPos());
            for (size_t i = 0; i < std::min(player.tracks.size(), trackMuted.size()); i++)
                player.tracks.at(i).muted = trackMuted[i];
            hasEnded = false;
            paused = false;
        }
        //paused = !paused;
    };

    InvokeAsPlayer(func);

    /* Return true if the player was paused an is now playing again. False otherwise. */
    return !paused;
}

void PlaybackEngine::Stop()
{
    auto func = [this]() {
        const uint8_t playerIdx = ctx->primaryPlayer;
        if (playerIdx >= ctx->players.size())
            return;

        ctx->m4aMPlayAllStop();
        ctx->m4aMPlayStart(playerIdx, ctx->players.at(playerIdx).GetSongHeaderPos());
        ctx->m4aMPlayStop(playerIdx);
        hasEnded = false;
        paused = false;
    };

    InvokeAsPlayer(func);
}

void PlaybackEngine::SpeedDouble()
{
    speedFactor <<= 1;
    if (speedFactor > 1024)
        speedFactor = 1024;

    auto func = [this]() {
        // TODO replace this with m4aMPlayTempoControl
        ctx->reader.SetSpeedFactor(float(speedFactor) / 64.0f);
    };

    InvokeAsPlayer(func);
}

void PlaybackEngine::SpeedHalve()
{
    speedFactor >>= 1;
    if (speedFactor < 1)
        speedFactor = 1;

    auto func = [this]() {
        // TODO replace this with m4aMPlayTempoControl
        ctx->reader.SetSpeedFactor(float(speedFactor) / 64.0f);
    };

    InvokeAsPlayer(func);
}

bool PlaybackEngine::HasEnded() const
{
    return hasEnded;
}

void PlaybackEngine::ToggleMute(size_t index)
{
    if (index >= trackMuted.size())
        return;

    trackMuted[index] = !trackMuted[index];

    auto func = [this, index]() {
        const uint8_t playerIdx = ctx->primaryPlayer;
        if (playerIdx >= ctx->players.size())
            return;
        MP2KPlayer &player = ctx->players.at(playerIdx);
        if (index >= player.tracks.size())
            return;
        MP2KTrack &trk = player.tracks.at(index);
        trk.muted = !trk.muted;
    };

    InvokeAsPlayer(func);
}

void PlaybackEngine::Mute(size_t index, bool mute)
{
    if (index >= trackMuted.size())
        return;

    trackMuted[index] = mute;

    auto func = [this, index, mute]() {
        const uint8_t playerIdx = ctx->primaryPlayer;
        if (playerIdx >= ctx->players.size())
            return;
        MP2KPlayer &player = ctx->players.at(playerIdx);
        if (index >= player.tracks.size())
            return;
        MP2KTrack &trk = player.tracks.at(index);
        trk.muted = mute;
    };

    InvokeAsPlayer(func);
}

SongInfo PlaybackEngine::GetSongInfo()
{
    SongInfo songInfo;

    auto func = [this, &songInfo]() {
        const MP2KPlayer &player = ctx->players.at(ctx->primaryPlayer);

        songInfo.songHeaderPos = player.GetSongHeaderPos();
        songInfo.voiceTablePos = player.GetSoundBankPos();
        songInfo.reverb = player.GetReverb();
        songInfo.priority = player.GetPriority();
        songInfo.playerIdx = player.playerIdx;
    };

    InvokeAsPlayer(func);

    return songInfo;
}

void PlaybackEngine::GetVisualizerState(MP2KVisualizerState &visualizerState)
{
    /* We do not use InvokeAsPlayer, as that would block the call until the background thread
     * makes the data available. Like this, the mutex only blocks for the actual copy. */
    std::scoped_lock l(visualizerStateMutex);

    visualizerState = visualizerStateObserver;
}

/*
 * private PlaybackEngine
 */

void PlaybackEngine::InitContext()
{
    ctx = std::make_unique<MP2KContext>(
        Rom::Instance(),
        profile.mp2kSoundModePlayback,
        profile.agbplaySoundMode,
        profile.songTableInfoPlayback,
        profile.playerTablePlayback
    );
}

void PlaybackEngine::threadWorker()
{
    std::vector<sample> silenceBuffer(ctx->mixer.GetSamplesPerBuffer());

    while (!playerThreadQuit) {
        /* Run events from main thread. */
        InvokeRun();

        if (paused) {
            /* Silence output buffer. */
            ringbuffer.Put(silenceBuffer);
        } else {
            /* Run sound engine. */
            ctx->m4aSoundMain();
            updateVisualizerState();

            /* Write audio data to portaudio ringbuffer. */
            assert(ctx->masterAudioBuffer.size() == ctx->mixer.GetSamplesPerBuffer());
            ringbuffer.Put(ctx->masterAudioBuffer);
        }

        /* Stop all players after loop fadeout (or end). */
        if (ctx->HasEnded() && !hasEnded) {
            ctx->m4aMPlayAllStop();
            hasEnded = true;
        }
    }
}

void PlaybackEngine::updateVisualizerState()
{
    /* We assume that GetVisualizerState may be more expensive than a simple copy.
     * Accordingly we generate the state, then we lock the mutex and publish the state.
     * That way the mutex is only locked for the smallest duration necessary. */

    ctx->GetVisualizerState(visualizerStatePlayer);

    std::scoped_lock l(visualizerStateMutex);

    visualizerStateObserver = visualizerStatePlayer;
}

void PlaybackEngine::InvokeAsPlayer(const std::function<void(void)> &func)
{
    /* This function must be called from within the main thread only! */
    assert(std::this_thread::get_id() != playerThread->get_id());

    std::unique_lock l(playerInvokeMutex);

    playerInvokePending = true;
    playerInvokeReady.wait(l);
    func();
    playerInvokePending = false;
    playerInvokeComplete.notify_one();
}

void PlaybackEngine::InvokeRun()
{
    /* This function must be called from within the player thread only! */
    //This does not work as playerThread is not fully initialized eventhough this thread is already running
    //assert(std::this_thread::get_id() == playerThread->get_id());

    if (!playerInvokePending)
        return;

    std::unique_lock l(playerInvokeMutex);
    playerInvokeReady.notify_one();

    /* wait for remote invocation to complete... */
    playerInvokeComplete.wait(l);
}

int PlaybackEngine::audioCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
    (void)inputBuffer;
    (void)timeInfo;
    (void)statusFlags;
    PlaybackEngine *_this = static_cast<PlaybackEngine *>(userData);
    _this->ringbuffer.Take({static_cast<sample *>(outputBuffer), framesPerBuffer});
    return paContinue;
}

void PlaybackEngine::portaudioOpen()
{
    auto &sys = portaudio::System::instance();

    // init host api
    std::vector<PaHostApiTypeId> hostApiPrioritiesWithFallback = hostApiPriority;
    const auto &defaultHostApi = sys.defaultHostApi();
    const auto f = std::find(hostApiPrioritiesWithFallback.begin(), hostApiPrioritiesWithFallback.end(), defaultHostApi.typeId());
    if (f == hostApiPrioritiesWithFallback.end())
        hostApiPrioritiesWithFallback.push_back(defaultHostApi.typeId());

    bool streamOpen = false;

    /* Loop over all wanted host APIs in the prioritized order and use the first one,
     * which can successfully open a stream with the default device. */
    for (const auto apiType : hostApiPrioritiesWithFallback) {
        const PaHostApiIndex hostApiIndex = Pa_HostApiTypeIdToHostApiIndex(apiType);
        if (hostApiIndex < 0)
            continue;

        const auto &currentHostApi = sys.hostApiByIndex(hostApiIndex);

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

        portaudio::DirectionSpecificStreamParameters outPars(
            currentHostApi.defaultOutputDevice(),
            2,  // stereo
            portaudio::SampleDataFormat::FLOAT32,
            true,
            currentHostApi.defaultOutputDevice().defaultLowOutputLatency(),
            hostApiSpecificStreamInfo.get()
        );

        portaudio::StreamParameters pars(
            portaudio::DirectionSpecificStreamParameters::null(),
            outPars,
            ctx->mixer.GetSampleRate(),
            paFramesPerBufferUnspecified,
            paNoFlag
        );

        try {
            audioStream.open(
                pars,
                audioCallback,
                static_cast<void *>(this)
            );

            audioStream.start();
        } catch (portaudio::Exception &e) {
            Debug::print("unable to open/start stream with Host API {}: {}", currentHostApi.name(), e.what());
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
    try {
        audioStream.stop();
        audioStream.close();
    } catch (portaudio::Exception &e) {
        Debug::print("Error while stopping/closing portaudio stream: {}", e.what());
    }
}
