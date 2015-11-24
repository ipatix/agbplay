#include "PlayerModule.h"

using namespace std;
using namespace agbplay;

/*
 * public DisplayData
 */

DisplayData::DisplayData() {
    trackPtr = 0;
    isCalling = false;
    isMuted = false;
    vol = 100;
    bendr = 2;
    pan = 0;
    prog = 0;
    bend = 0;
    tune = 0;
    envL = 0;
    envR = 0;
    delay = 0;
    activeNotes.reset();
}

DisplayData::~DisplayData() {
    // empty
}

/*
 * public DisplayContainer
 */

DisplayContainer::DisplayContainer() {
    // empty
}

DisplayContainer::DisplayContainer(uint8_t nTracks) {
    data = vector<DisplayData>(nTracks);
}

DisplayContainer::~DisplayContainer() {
    // empty
}

/*
 * public PlayerModule
 */

PlayerModule::PlayerModule(Rom& rrom, TrackviewGUI *trackUI, uint16_t initSong) : rom(rrom) {
    this->trackUI = trackUI;
    LoadSong(initSong);
    loadedSong = initSong;
}

PlayerModule::~PlayerModule() {

}

void PlayerModule::LoadSong(uint16_t songNum) {

}

/*
 * private PlayerModule
 */
