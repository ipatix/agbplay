#pragma once

#include <cstdint>
#include <functional>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <portaudiocpp/PortAudioCpp.hxx>

#include "Constants.h"
#include "GameConfig.h"
#include "Ringbuffer.h"
#include "LoudnessCalculator.h"
#include "MP2KContext.h"

class PlaybackEngine 
{
public:
    PlaybackEngine(const SongTableInfo &songTableInfo, const PlayerTableInfo &playerTableInfo);
    PlaybackEngine(const PlaybackEngine&) = delete;
    PlaybackEngine& operator=(const PlaybackEngine&) = delete;
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
    void InitContext();
    void threadWorker();
    void updateVisualizerState();
    void InvokeAsPlayer(const std::function<void(void)> &func);
    void InvokeRun();
    static int audioCallback(const void *inputBuffer, void *outputBuffer, size_t framesPerBuffer,
            const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags,
            void *userData);

    void portaudioOpen();
    void portaudioClose();

    static const std::vector<PaHostApiTypeId> hostApiPriority;

    portaudio::FunCallbackStream audioStream;
    uint32_t speedFactor = 64;
    bool paused = false;
    std::unique_ptr<MP2KContext> ctx;
    Ringbuffer rBuf{STREAM_BUF_SIZE};

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

    const SongTableInfo &songTableInfo;
    const PlayerTableInfo &playerTableInfo;
    uint16_t songIdx = 0;
};
