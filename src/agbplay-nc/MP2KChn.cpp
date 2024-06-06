#include "MP2KChn.h"

#include <cassert>

#include "MP2KTrack.h"

MP2KChn::MP2KChn(MP2KTrack *track, const Note &note, const ADSR &env) : note(note), env(env)
{
    AddToTrack(track);
}

MP2KChn::~MP2KChn()
{
    RemoveFromTrack();
}

void MP2KChn::AddToTrack(MP2KTrack *track) noexcept
{
    assert(this->track == nullptr);
    this->track = track;
    assert(this->trackOrg == nullptr);
    this->trackOrg = track;

    prev = nullptr;
    next = track->channels;

    if (track->channels)
        track->channels->prev = this;

    track->channels = this;
}

void MP2KChn::RemoveFromTrack() noexcept
{
    if (track == nullptr)
        return;

    if (prev)
        prev->next = next;
    else
        track->channels = next;

    if (next)
        next->prev = prev;

    // Do not clear pointers in order to not break for loops over the channel list.
    // This possibly leads to dangling pointers!
    // Do not use these pointers if track == nullptr.
    //prev = nullptr;
    //next = nullptr;
    track = nullptr;
}

bool MP2KChn::IsReleasing() const noexcept
{
    return stop;
}
