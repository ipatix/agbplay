#include "PlayerModule.h"

using namespace std;
using namespace agbplay;

/*
 * public PlayerModule
 */

PlayerModule::PlayerModule(Rom& rrom, TrackviewGUI *trackUI, long initSongPos) : seq(Sequence(initSongPos, &rrom)), rom(rrom)
{
    this->trackUI = trackUI;
}

PlayerModule::~PlayerModule() 
{
}

void PlayerModule::LoadSong(long songPos)
{
    seq = Sequence(songPos, &rom);
    trackUI->SetState(seq.GetUpdatedDisp());
}

/*
 * private PlayerModule
 */
