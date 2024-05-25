#pragma once

#include <cstdint>
#include <vector>
#include <thread>
#include <memory>
#include <portaudiocpp/PortAudioCpp.hxx>

#include "PlaybackSongState.h"
#include "Constants.h"
#include "GameConfig.h"
#include "Ringbuffer.h"
#include "LoudnessCalculator.h"
#include "MP2KContext.h"

class PlaybackEngine 
{
public:
    PlaybackEngine(size_t initSongPos);
    PlaybackEngine(const PlaybackEngine&) = delete;
    PlaybackEngine& operator=(const PlaybackEngine&) = delete;
    ~PlaybackEngine();

    void LoadSong(size_t songPos);
    void Play();
    void Pause();
    void Stop();
    void SpeedDouble();
    void SpeedHalve();
    bool IsPlaying();
    bool IsPaused() const;
    void ToggleMute(size_t index);
    void Mute(size_t index, bool mute);
    size_t GetMaxTracks() { return mutedTracks.size(); }
    SongInfo GetSongInfo() const;
    const PlaybackSongState &GetPlaybackSongState() const;

private:
    void initContext();
    void threadWorker();
    void updatePlaybackState(bool reset = false);
    static int audioCallback(const void *inputBuffer, void *outputBuffer, size_t framesPerBuffer,
            const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags,
            void *userData);

    void setupLoudnessCalcs();
    void portaudioOpen();
    void portaudioClose();

    static const std::vector<PaHostApiTypeId> hostApiPriority;

    portaudio::FunCallbackStream audioStream;
    uint32_t speedFactor = 64;
    volatile enum class State : int {
        RESTART, PLAYING, PAUSED, TERMINATED, SHUTDOWN, THREAD_DELETED
    } playerState = State::THREAD_DELETED;
    std::unique_ptr<MP2KContext> ctx;
    Ringbuffer rBuf{STREAM_BUF_SIZE};

    LoudnessCalculator masterLoudness{10.0f};
    PlaybackSongState songState;
    std::vector<LoudnessCalculator> trackLoudness;
    std::vector<bool> mutedTracks;

    std::unique_ptr<std::thread> playerThread;
};
