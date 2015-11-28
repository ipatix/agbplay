#include "DisplayContainer.h"

using namespace agbplay;
using namespace std;

/*
 * public DisplayData
 */

DisplayData::DisplayData() 
{
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

DisplayData::~DisplayData() 
{
}

/*
 * public DisplayContainer
 */

// TODO move to seperate file

DisplayContainer::DisplayContainer() 
{
}

DisplayContainer::DisplayContainer(uint8_t nTracks) 
{
    data = vector<DisplayData>(nTracks);
}

DisplayContainer::~DisplayContainer() 
{
    // empty
}

