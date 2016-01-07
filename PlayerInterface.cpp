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
    this->playerState = State::STOPPED;
}

PlayerInterface::~PlayerInterface() 
{
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
            break;
        case State::PLAYING:
            // restart song if player is running
            playerState = State::RESTART;
            break;
        case State::PAUSED:
            // continue paused playback
            playerState = State::PLAYING;
            break;
        case State::STOPPING:
            break;
        case State::STOPPED:
            break;
    }
}

void PlayerInterface::Pause()
{
    switch (playerState) {
        case State::RESTART:
            break;
        case State::PLAYING:
            break;
        case State::PAUSED:
            break;
        case State::STOPPING:
            break;
        case State::STOPPED:
            break;
    }
}

void PlayerInterface::Unpause()
{
    switch (playerState) {
        case State::RESTART:
            break;
        case State::PLAYING:
            break;
        case State::PAUSED:
            break;
        case State::STOPPING:
            break;
        case State::STOPPED:
            break;
    }
}

void PlayerInterface::Stop()
{
    switch (playerState) {
        case State::RESTART:
            break;
        case State::PLAYING:
            break;
        case State::PAUSED:
            break;
        case State::STOPPING:
            break;
        case State::STOPPED:
            break;
    }
}

/*
 * private PlayerInterface
 */
