#pragma once

#include <map>
#include <vector>

#include "Constants.h"
#include "SoundData.h"
#include "SoundMixer.h"

struct PlayerContext;

class SequenceReader
{
public:
    SequenceReader(PlayerContext& ctx, int8_t maxLoops);
    SequenceReader(const SequenceReader&) = delete;
    SequenceReader& operator=(const SequenceReader&) = delete;

    void Process(bool liveMode);
    bool EndReached() const;
    void Restart();
    void SetSpeedFactor(float speedFactor);
    void PlayLiveNoteOn(uint8_t key, uint8_t vel, uint8_t trackIdx);
    void PlayLiveNoteOff(uint8_t key, uint8_t trackIdx);
    void PlayLiveCommand(
            uint8_t cmd, uint8_t uarg, int8_t sarg, uint8_t trackIdx);

    static const std::vector<uint32_t> freqLut;

private:
    static const std::map<uint8_t, int8_t> delayLut;
    static const std::map<uint8_t, int8_t> noteLut;

    PlayerContext& ctx;

    bool endReached = false;
    const int8_t maxLoops = 1;
    uint8_t numLoops = 0;
    float speedFactor = 1.0f;

    int tickTrackNotes(uint8_t track_idx, std::bitset<NUM_NOTES>& activeNotes);
    // for live notes we need to keep track of notes by channel/key -> note ID
    std::map<std::pair<uint8_t, uint8_t>, uint32_t> liveNotes;
    uint32_t liveNoteID = 0;

    void processSequenceTick(bool liveMode);
    void setTrackPV(uint8_t track_idx, uint8_t vol, int8_t pan, int16_t pitch);

    void cmdPlayNote(uint8_t cmd, uint8_t trackIdx);
    void cmdPlayCommand(uint8_t cmd, uint8_t trackIdx);
    void enqueueNote(
            uint8_t key, uint8_t vel, uint8_t len, uint8_t trackIdx,
            bool liveMode);

    void cmdPlayFine(uint8_t trackIdx);
    void cmdPlayMemacc(uint8_t trackIdx);
    void cmdPlayXCmd(uint8_t trackIdx);
};
