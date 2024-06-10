#include "MP2KPlayer.h"

#include "MP2KTrack.h"
#include "ReverbEffect.h"
#include "Rom.h" // TODO remove once Rom is deglobalized

MP2KPlayer::MP2KPlayer(uint8_t trackLimit, uint8_t playerIdx)
    : playerIdx(playerIdx), trackLimit(trackLimit)
{
    for (uint8_t i = 0; i < trackLimit; i++)
        tracks.emplace_back(i);
    Init(0);
}

void MP2KPlayer::Init(size_t songHeaderPos)
{
    Rom& rom = Rom::Instance();

    this->songHeaderPos = songHeaderPos;

    if (songHeaderPos != 0) {
        // read song header
        tracksUsed = std::min<uint8_t>(rom.ReadU8(songHeaderPos + 0), trackLimit);

        // read track pointers
        for (size_t i = 0; i < tracksUsed; i++)
            tracks.at(i).Init(rom.ReadAgbPtrToPos(songHeaderPos + 8 + 4 * i));

        enabled = true;
    } else {
        tracksUsed = 0;
        enabled = false;
    }

    // reset rest of the tracks
    for (size_t i = tracksUsed; i < tracks.size(); i++)
        tracks.at(i).Init(0);

    // reset runtime variables
    bpmStack = 0;
    bpm = 150;
    tickCount = 0;
}

void MP2KPlayer::Reset()
{
    Init(songHeaderPos);
}

size_t MP2KPlayer::GetSoundBankPos() const
{
    if (songHeaderPos == 0)
        return 0;

    Rom& rom = Rom::Instance();
    /* Sometimes songs have 0 tracks and will not have a valid
     * sound bank pointer. Return a dummy result instead since
     * it should not be accessed anyway */
    if (!rom.ValidPointer(rom.ReadU32(songHeaderPos + 4)))
        return 0;

    return rom.ReadAgbPtrToPos(songHeaderPos + 4);
}

uint8_t MP2KPlayer::GetReverb() const
{
    if (songHeaderPos == 0)
        return 0;

    return Rom::Instance().ReadU8(songHeaderPos + 3);
}

uint8_t MP2KPlayer::GetPriority() const
{
    if (songHeaderPos == 0)
        return 0;

    return Rom::Instance().ReadU8(songHeaderPos + 2);
}

size_t MP2KPlayer::GetSongHeaderPos() const
{
    return songHeaderPos;
}
