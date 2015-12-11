#include "PlayerInterface.h"

using namespace std;
using namespace agbplay;

/*
 * public PlayerInterface
 */

PlayerInterface::PlayerInterface(Rom& _rom, TrackviewGUI *trackUI, long initSongPos, EnginePars pars
        ) : seq(Sequence(initSongPos, 16, _rom)), rom(_rom)
{
    this->trackUI = trackUI;
    this->dSoundVol = pars.vol + 1;
    this->dSoundRev = pars.rev;
    this->dSoundFreq = (pars.freq == 0 || pars.freq > 12) ? freqLut[4] : freqLut[pars.freq - 1];
}

PlayerInterface::~PlayerInterface() 
{
}

void PlayerInterface::LoadSong(long songPos, uint8_t trackLimit)
{
    seq = Sequence(songPos, trackLimit, rom);
    trackUI->SetState(seq.GetUpdatedDisp());
}

/*
 * private PlayerInterface
 */
