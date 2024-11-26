#pragma once

#include "LowLatencyRingbuffer.hpp"
#include "MP2KContext.hpp"
#include "Profile.hpp"

#include <bitset>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <portaudiocpp/PortAudioCpp.hxx>
#include <thread>
#include <vector>

class PlaybackEngine
{
public:
    PlaybackEngine(uint32_t sampleRate, const Profile &profile);
    PlaybackEngine(const PlaybackEngine &) = delete;
    PlaybackEngine &operator=(const PlaybackEngine &) = delete;
    ~PlaybackEngine();

    void LoadSong(uint16_t songIdx);
    void Play();
    bool Pause();
    void Stop();
    void SpeedDouble();
    void SpeedHalve();
    bool HasEnded() const;
    void ToggleMute(size_t index);
    void Mute(size_t index, bool mute);
    SongInfo GetSongInfo();
    void GetVisualizerState(MP2KVisualizerState &visualizerState);

private:
    void threadWorker();
    void updateVisualizerState();
    void InvokeAsPlayer(const std::function<void(void)> &func);
    void InvokeRun();
    static int audioCallback(
        const void *inputBuffer,
        void *outputBuffer,
        unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo *timeInfo,
        PaStreamCallbackFlags statusFlags,
        void *userData
    );

    void portaudioOpen();
    void portaudioClose();

    static const std::vector<PaHostApiTypeId> hostApiPriority;

    portaudio::FunCallbackStream audioStream;
    uint32_t speedFactor = 64;
    std::bitset<16> trackMuted;    // TODO replace 16 with constant
    bool paused = false;
    std::unique_ptr<MP2KContext> ctx;

    MP2KVisualizerState visualizerStatePlayer;
    MP2KVisualizerState visualizerStateObserver;
    std::mutex visualizerStateMutex;

    std::mutex playerInvokeMutex;
    std::condition_variable playerInvokeReady;
    std::condition_variable playerInvokeComplete;
    std::atomic<bool> playerInvokePending = false;
    std::unique_ptr<std::thread> playerThread;
    std::atomic<bool> playerThreadQuit = false;

    std::atomic<bool> hasEnded = false;

    LowLatencyRingbuffer ringbuffer;
    // std::vector<sample> outputBuffer;
    // std::mutex outputBufferMutex;
    // std::condition_variable outputBufferReady;
    // bool outputBufferValid = false;

    const Profile &profile;
    uint16_t songIdx = 0;
};
