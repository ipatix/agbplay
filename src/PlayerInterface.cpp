#include <thread>
#include <chrono>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <cassert>

#include "PlayerInterface.h"
#include "Xcept.h"
#include "Debug.h"
#include "Util.h"
#include "ConfigManager.h"

/*
 * PlayerInterface data
 */

// first portaudio hostapi has highest priority, last hostapi has lowest
// if none are available, the default one is selected.
// they are also the ones which are known to work
const std::vector<PaHostApiTypeId> PlayerInterface::hostApiPriority = {
    // Unix
    paJACK,
    paALSA,
    // Windows
    paMME, // only option for cygwin
};

/*
 * public PlayerInterface
 */

PlayerInterface::PlayerInterface(
        TrackviewGUI &trackUI, size_t initSongPos, int midiPortNumber)
    : trackUI(trackUI),
    mutedTracks(ConfigManager::Instance().GetCfg().GetTrackLimit())
{
    rtmidiOpen(midiPortNumber);
    initContext();
    ctx->InitSong(initSongPos, midiin != nullptr);
    setupLoudnessCalcs();
    portaudioOpen();
    if (midiin != nullptr) {
        Play();
    }
}

PlayerInterface::~PlayerInterface() 
{
    // stop and deallocate player thread if required
    Stop();
    rtmidiClose();
    portaudioClose();
}

void PlayerInterface::LoadSong(size_t songPos)
{
    bool play = playerState == State::PLAYING;
    Stop();
    ctx->InitSong(songPos, midiin != nullptr);
    setupLoudnessCalcs();
    // TODO replace this with pairs
    float vols[ctx->seq.tracks.size() * 2];
    for (size_t i = 0; i < ctx->seq.tracks.size() * 2; i++)
        vols[i] = 0.0f;

    trackUI.SetState(ctx->seq, vols, 0, 0);

    if (play)
        Play();
}

void PlayerInterface::Play()
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
        playerThread = std::make_unique<std::thread>(&PlayerInterface::threadWorker, this);
#ifdef __linux__
        pthread_setname_np(playerThread->native_handle(), "mixer thread");
#endif
        // start thread and play back song
        break;
    }
}

void PlayerInterface::Pause()
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

void PlayerInterface::Stop()
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

void PlayerInterface::SpeedDouble()
{
    speedFactor <<= 1;
    if (speedFactor > 1024)
        speedFactor = 1024;
    ctx->reader.SetSpeedFactor(float(speedFactor) / 64.0f);
}

void PlayerInterface::SpeedHalve()
{
    speedFactor >>= 1;
    if (speedFactor < 1)
        speedFactor = 1;
    ctx->reader.SetSpeedFactor(float(speedFactor) / 64.0f);
}

bool PlayerInterface::IsPlaying()
{
    return playerState != State::THREAD_DELETED && playerState != State::TERMINATED;
}

void PlayerInterface::UpdateView()
{
    if (playerState != State::THREAD_DELETED &&
            playerState != State::SHUTDOWN &&
            playerState != State::TERMINATED) {
        size_t trks = ctx->seq.tracks.size();
        assert(trks == trackLoudness.size());
        float vols[trks * 2];
        for (size_t i = 0; i < trks; i++)
            trackLoudness[i].GetLoudness(vols[i*2], vols[i*2+1]);

        /* count number of active PCM channels */
        trackUI.SetState(ctx->seq, vols, static_cast<int>(ctx->sndChannels.size()), -1);
    }
}

void PlayerInterface::ToggleMute(size_t index)
{
    mutedTracks[index] = !mutedTracks[index];
}

void PlayerInterface::Mute(size_t index, bool mute)
{
    mutedTracks[index] = mute;
}

void PlayerInterface::GetMasterVolLevels(float& left, float& right)
{
    masterLoudness.GetLoudness(left, right);
}

SongInfo PlayerInterface::GetSongInfo() const
{
    SongInfo result;
    result.songHeaderPos = ctx->seq.GetSongHeaderPos();
    result.voiceTablePos = ctx->seq.GetSoundBankPos();
    result.reverb = ctx->seq.GetReverb();
    result.priority = ctx->seq.GetPriority();
    return result;
}

/*
 * private PlayerInterface
 */

void PlayerInterface::initContext()
{
    const auto& cfg = ConfigManager::Instance().GetCfg();

    /* We could make the context a member variable instead of
     * a unique_ptr, but initialization get's a little messy that way */
    ctx = std::make_unique<PlayerContext>(
            ConfigManager::Instance().GetMaxLoopsPlaylist(),
            cfg.GetTrackLimit(),
            EnginePars(cfg.GetPCMVol(), cfg.GetEngineRev(), cfg.GetEngineFreq()),
            midiin
            );
}

void PlayerInterface::threadWorker()
{
    size_t samplesPerBuffer = ctx->mixer.GetSamplesPerBuffer();
    std::vector<sample> silence(samplesPerBuffer, sample{0.0f, 0.0f});
    std::vector<sample> masterAudio(samplesPerBuffer, sample{0.0f, 0.0f});
    std::vector<std::vector<sample>> trackAudio;
    try {
        while (playerState != State::SHUTDOWN) {
            switch (playerState) {
            case State::RESTART:
                if (midiin != nullptr) {
                    ctx->KillAllChannels();
                } else {
                    ctx->InitSong(
                            ctx->seq.GetSongHeaderPos(), midiin != nullptr);
                }
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
                throw Xcept("Internal PlayerInterface error: %d", (int)playerState);
            }
        }
        // reset song state after it has finished
    } catch (std::exception& e) {
        ctx->InitSong(ctx->seq.GetSongHeaderPos(), midiin != nullptr);
        Debug::print("FATAL ERROR on streaming thread: %s", e.what());
    }
    masterLoudness.Reset();
    for (LoudnessCalculator& c : trackLoudness)
        c.Reset();
    // flush buffer
    rBuf.Clear();
    playerState = State::TERMINATED;
}

int PlayerInterface::audioCallback(const void *inputBuffer, void *outputBuffer, size_t framesPerBuffer,
        const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
    (void)inputBuffer;
    (void)timeInfo;
    (void)statusFlags;
    Ringbuffer *rBuf = (Ringbuffer *)userData;
    rBuf->Take((sample *)outputBuffer, framesPerBuffer);
    return 0;
}

void PlayerInterface::setupLoudnessCalcs()
{
    trackLoudness.clear();
    for (size_t i = 0; i < ctx->seq.tracks.size(); i++)
        trackLoudness.emplace_back(5.0f);
}

void PlayerInterface::portaudioOpen()
{
    // init host api
    PaDeviceIndex deviceIndex = -1;
    PaHostApiIndex hostApiIndex = -1;
    for (const auto apiType : hostApiPriority) {
        hostApiIndex = Pa_HostApiTypeIdToHostApiIndex(apiType);
        // prioritized host api available ?
        if (hostApiIndex < 0)
            continue;

        const PaHostApiInfo *apiinfo = Pa_GetHostApiInfo(hostApiIndex);
        if (apiinfo == NULL)
            throw Xcept("Pa_GetHostApiInfo with valid index failed");
        deviceIndex = apiinfo->defaultOutputDevice;
        break;
    }
    if (hostApiIndex < 0) {
        // no prioritized api was found, use default
        const PaHostApiInfo *apiinfo = Pa_GetHostApiInfo(Pa_GetDefaultHostApi());
        Debug::print("No supported API found, falling back to: %s", apiinfo->name);
        if (apiinfo == NULL)
            throw Xcept("Pa_GetHostApiInfo with valid index failed");
        deviceIndex = apiinfo->defaultOutputDevice;
    }

    const PaDeviceInfo *devinfo = Pa_GetDeviceInfo(deviceIndex);
    if (devinfo == NULL)
        throw Xcept("Pa_GetDeviceInfo with valid index failed");

    PaStreamParameters outputStreamParameters;
    outputStreamParameters.device = deviceIndex;
    outputStreamParameters.channelCount = 2;    // stereo
    outputStreamParameters.sampleFormat = paFloat32;
    outputStreamParameters.suggestedLatency = devinfo->defaultLowOutputLatency;
    outputStreamParameters.hostApiSpecificStreamInfo = NULL;

    PaError err;
    uint32_t outSampleRate = ctx->mixer.GetSampleRate();
    if ((err = Pa_OpenStream(&audioStream, NULL, &outputStreamParameters, outSampleRate, 0, paNoFlag, audioCallback, (void *)&rBuf)) != paNoError) {
        Debug::print("Pa_OpenDefaultStream: %s", Pa_GetErrorText(err));
        return;
    }
    if ((err = Pa_StartStream(audioStream)) != paNoError) {
        Debug::print("PA_StartStream: %s", Pa_GetErrorText(err));
        return;
    }
}

void PlayerInterface::portaudioClose()
{
    PaError err;
    if ((err = Pa_StopStream(audioStream)) != paNoError) {
        Debug::print("Pa_StopStream: %s", Pa_GetErrorText(err));
    }
    if ((err = Pa_CloseStream(audioStream)) != paNoError) {
        Debug::print("Pa_CloseStream: %s", Pa_GetErrorText(err));
    }
}

// void midicallback(
//         double deltatime, std::vector<unsigned char> *message, void
//         *userData)
// {
//     unsigned int nBytes = message->size();
//     for (unsigned int i = 0; i < nBytes; i++)
//         std::cout << "Byte " << i << " = " << (int)message->at(i) << ", ";
//     if (nBytes > 0)
//         std::cout << "stamp = " << deltatime << std::endl;
// }

void PlayerInterface::rtmidiOpen(int portNumber)
{
    if (portNumber < 0) {
        midiin = nullptr;
        return;
    }

    midiin = new RtMidiIn(RtMidi::UNSPECIFIED, "agbsnd", 65536);
    // Check inputs.
    unsigned int nPorts = midiin->getPortCount();
    // std::cout << "\nThere are " << nPorts << " MIDI input sources
    // available.\n";
    if (portNumber >= nPorts) {
        Debug::print(
                "Cannot open MIDI input: Invalid port number %d", portNumber);
        delete midiin;
        midiin = nullptr;
        return;
    }
    try {
        midiPortName = midiin->getPortName(portNumber);
        Debug::print("Using MIDI input: %s", midiPortName.c_str());
        midiin->openPort(portNumber);

    } catch (RtMidiError &error) {
        Debug::print("Cannot open MIDI input: %s", error.getMessage().c_str());
        delete midiin;
        midiin = nullptr;
        return;
    }
}

void PlayerInterface::rtmidiClose() { delete midiin; }