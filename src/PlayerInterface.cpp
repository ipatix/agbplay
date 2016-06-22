#include <thread>
#include <chrono>
#include <cstdlib>
#include <algorithm>
#include <cmath>

#include "PlayerInterface.h"
#include "MyException.h"
#include "Debug.h"
#include "Util.h"

using namespace std;
using namespace agbplay;

/*
 * public PlayerInterface
 */

PlayerInterface::PlayerInterface(Rom& _rom, TrackviewGUI *trackUI, long initSongPos, GameConfig& _gameCfg) 
    : rom(_rom), gameCfg(_gameCfg), seq(initSongPos, _gameCfg.GetTrackLimit(), _rom), 
    rBuf(N_CHANNELS * STREAM_BUF_SIZE), masterLoudness(20.f)
{
    this->trackUI = trackUI;
    playerState = State::THREAD_DELETED;
    speedFactor = 64;

    sg = new StreamGenerator(seq, EnginePars(gameCfg.GetPCMVol(), gameCfg.GetEngineRev(), gameCfg.GetEngineFreq()), 1, float(speedFactor) / 64.0f, gameCfg.GetRevType());
    setupLoudnessCalcs();
    // start audio stream
    PaError err;
    //uint32_t nBlocks = sg->GetBufferUnitCount();
    uint32_t outSampleRate = sg->GetRenderSampleRate();
    if ((err = Pa_OpenDefaultStream(&audioStream, 0, N_CHANNELS, paFloat32, outSampleRate, /*nBlocks * N_CHANNELS*/0, audioCallback, (void *)&rBuf)) != paNoError) {
        __print_debug(FormatString("Pa_OpenDefaultStream: %s", Pa_GetErrorText(err)));
        return;
    }
    if ((err = Pa_StartStream(audioStream)) != paNoError) {
        __print_debug(FormatString("PA_StartStream: %s", Pa_GetErrorText(err)));
        return;
    }
}

PlayerInterface::~PlayerInterface() 
{
    // stop and deallocate player thread if required
    Stop();
    PaError err;
    if ((err = Pa_StopStream(audioStream)) != paNoError) {
        __print_debug(FormatString("Pa_StopStream: %s", Pa_GetErrorText(err)));
    }
    if ((err = Pa_CloseStream(audioStream)) != paNoError) {
        __print_debug(FormatString("Pa_CloseStream: %s", Pa_GetErrorText(err)));
    }
    delete sg;
}

void PlayerInterface::LoadSong(long songPos)
{
    bool play = playerState == State::PLAYING;
    Stop();
    seq = Sequence(songPos, gameCfg.GetTrackLimit(), rom);
    setupLoudnessCalcs();
    float vols[seq.tracks.size() * N_CHANNELS];
    for (size_t i = 0; i < seq.tracks.size() * N_CHANNELS; i++)
        vols[i] = 0.0f;

    trackUI->SetState(seq, vols);
    delete sg;
    sg = new StreamGenerator(seq, EnginePars(gameCfg.GetPCMVol(), gameCfg.GetEngineRev(), gameCfg.GetEngineFreq()), 1, float(speedFactor) / 64.0f, gameCfg.GetRevType());
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
            playerThread = new boost::thread(&PlayerInterface::threadWorker, this);
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
                this_thread::sleep_for(chrono::milliseconds(5));
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
            delete playerThread;
            playerState = State::THREAD_DELETED;
            delete sg;
            sg = new StreamGenerator(seq, EnginePars(gameCfg.GetPCMVol(), gameCfg.GetEngineRev(), gameCfg.GetEngineFreq()), 1, float(speedFactor) / 64.0f, gameCfg.GetRevType());
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
    sg->SetSpeedFactor(float(speedFactor) / 64.0f);
}

void PlayerInterface::SpeedHalve()
{
    speedFactor >>= 1;
    if (speedFactor < 1)
        speedFactor = 1;
    sg->SetSpeedFactor(float(speedFactor) / 64.0f);
}

bool PlayerInterface::IsPlaying()
{
    return playerState != State::THREAD_DELETED && playerState != State::TERMINATED;
}

void PlayerInterface::UpdateView()
{
    if (playerState != State::THREAD_DELETED && playerState != State::SHUTDOWN && playerState != State::TERMINATED)
    {
        size_t trks = sg->GetWorkingSequence().tracks.size();
        assert(trks == trackLoudness.size());
        float vols[trks * N_CHANNELS];
        for (size_t i = 0; i < trks; i++)
            trackLoudness[i].GetLoudness(vols[i*N_CHANNELS], vols[i*N_CHANNELS+1]);
        trackUI->SetState(sg->GetWorkingSequence(), vols);
    }
}

void PlayerInterface::GetMasterVolLevels(float& left, float& right)
{
    masterLoudness.GetLoudness(left, right);
}

/*
 * private PlayerInterface
 */

void PlayerInterface::threadWorker()
{
    uint32_t nBlocks = sg->GetBufferUnitCount();
    vector<float> silence(nBlocks * N_CHANNELS, 0.0f);
    vector<float> audio(nBlocks * N_CHANNELS, 0.0f);
    try {
        // FIXME seems to still have an issue with a race condition and default case occuring
        while (playerState != State::SHUTDOWN) {
            switch (playerState) {
                case State::RESTART:
                    delete sg;
                    sg = new StreamGenerator(seq, EnginePars(gameCfg.GetPCMVol(), gameCfg.GetEngineRev(), gameCfg.GetEngineFreq()), 1, float(speedFactor) / 64.0f, gameCfg.GetRevType());
                    playerState = State::PLAYING;
                case State::PLAYING:
                    {
                        // clear high level mixing buffer
                        fill(audio.begin(), audio.end(), 0.0f);
                        // render audio buffers for tracks
                        vector<vector<float>>& raudio = sg->ProcessAndGetAudio();
                        for (size_t i = 0; i < raudio.size(); i++)
                        {
                            assert(raudio[i].size() == audio.size());
                            for (size_t j = 0; j < audio.size(); j++)
                                audio[j] += raudio[i][j];
                            trackLoudness[i].CalcLoudness(raudio[i].data(), nBlocks);
                        }
                        if (sg->HasStreamEnded())
                        {
                            playerState = State::SHUTDOWN;
                            break;
                        }
                        rBuf.Put(audio.data(), audio.size());
                        masterLoudness.CalcLoudness(audio.data(), nBlocks);
                    }
                    break;
                case State::PAUSED:
                    rBuf.Put(silence.data(), uint32_t(silence.size()));
                    break;
                default:
                    throw MyException(FormatString("Internal PlayerInterface error: %d", (int)playerState));
            }
        }
    } catch (exception& e) {
        __print_debug(FormatString("FATAL ERROR on streaming thread: %s", e.what()));
    }
    masterLoudness.Reset();
    for (LoudnessCalculator& c : trackLoudness)
        c.Reset();
    // flush buffer
    rBuf.Clear();
    playerState = State::TERMINATED;
}

int PlayerInterface::audioCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
    (void)inputBuffer;
    (void)timeInfo;
    (void)statusFlags;
    Ringbuffer *rBuf = (Ringbuffer *)userData;
    rBuf->Take((float *)outputBuffer, uint32_t(framesPerBuffer * N_CHANNELS));
    return 0;
}

void PlayerInterface::setupLoudnessCalcs()
{
    trackLoudness.clear();
    for (size_t i = 0; i < seq.tracks.size(); i++)
        trackLoudness.emplace_back(10.0f);
}
