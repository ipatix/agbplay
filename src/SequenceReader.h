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

    void Process();
    bool EndReached() const;
    void Restart();
    void SetSpeedFactor(float speedFactor);

    static const std::vector<uint32_t> freqLut;

private:
    static const std::map<uint8_t, int8_t> delayLut;
    static const std::map<uint8_t, int8_t> noteLut;

    PlayerContext& ctx;

    bool endReached = false;
    const int8_t maxLoops = 1;
    uint8_t numLoops = 0;
    float speedFactor = 1.0f;

    void processSequenceTick();
    int tickTrackNotes(uint8_t track_idx, std::bitset<NUM_NOTES>& activeNotes);
    void setTrackPV(uint8_t track_idx, uint8_t vol, int8_t pan, int16_t pitch, bool updateVolume, bool updatePitch);

    void cmdPlayNote(uint8_t cmd, uint8_t trackIdx);
    void cmdPlayCommand(uint8_t cmd, uint8_t trackIdx);

    void cmdPlayFine(uint8_t trackIdx);
    void cmdPlayMemacc(uint8_t trackIdx);
    void cmdPlayXCmd(uint8_t trackIdx);
};
