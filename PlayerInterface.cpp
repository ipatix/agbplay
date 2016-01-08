#include <thread>
#include <chrono>
#include <boost/bind.hpp>

#include "PlayerInterface.h"

using namespace std;
using namespace agbplay;

const vector<uint32_t> PlayerInterface::freqLut = 
{
    5734, 7884, 10512, 13379,
    15768, 18157, 21024, 26758,
    31536, 36314, 40137, 42048
};

/*
 * public PlayerInterface
 */

PlayerInterface::PlayerInterface(Rom& _rom, TrackviewGUI *trackUI, long initSongPos, EnginePars pars) 
: seq(Sequence(initSongPos, 16, _rom)), rom(_rom)
{
    this->trackUI = trackUI;
    this->dSoundVol = uint8_t(pars.vol + 1);
    this->dSoundRev = pars.rev;
    this->dSoundFreq = (pars.freq == 0 || pars.freq > 12) ? freqLut[4] : freqLut[pars.freq - 1];
    this->playerState = State::THREAD_DELETED;
}

PlayerInterface::~PlayerInterface() 
{
    // stop and deallocate player thread if required
    Stop();
}

void PlayerInterface::LoadSong(long songPos, uint8_t trackLimit)
{
    seq = Sequence(songPos, trackLimit, rom);
    trackUI->SetState(seq.GetUpdatedDisp());
    if (playerState == State::PLAYING) {
        Play();
    }
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
            playerState = State::RESTART;
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
            playerState = State::RESTART;
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
            // ignore this
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
            playerThread->join();
            delete playerThread;
            playerState = State::THREAD_DELETED;
            break;
        case State::SHUTDOWN:
            // incase stop needs to be done force stop
            playerThread->join();
            delete playerThread;
            playerState = State::THREAD_DELETED;
            break;            
        case State::THREAD_DELETED:
            // ignore this
            break;
    }
}

/*
 * private PlayerInterface
 */

void PlayerInterface::threadWorker()
{
    // TODO process data
}
