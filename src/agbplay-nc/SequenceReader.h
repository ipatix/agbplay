#pragma once

#include <map>
#include <vector>

#include "Constants.h"
#include "SoundData.h"
#include "SoundMixer.h"

struct MP2KContext;
struct MP2KTrack;
struct MP2KPlayer;

class SequenceReader
{
public:
    SequenceReader(MP2KContext& ctx);
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

    MP2KContext& ctx;

    bool endReached = false;
    uint8_t numLoops = 0;
    float speedFactor = 1.0f;

    bool PlayerMain(MP2KPlayer &player);
    bool TrackMain(MP2KPlayer &player, MP2KTrack &trk);
    void TrackVolPitchMain(MP2KTrack &trk);
    void TrackVolPitchSet(MP2KTrack &trk, uint16_t vol, int16_t pan, int16_t pitch, bool updateVolume, bool updatePitch);
    int TickTrackNotes(MP2KTrack &trk);

    void cmdPlayNote(MP2KPlayer &player, MP2KTrack &trk, uint8_t cmd);
    void cmdPlayCommand(MP2KPlayer &player, MP2KTrack &trk, uint8_t cmd);

    void cmdPlayFine(MP2KTrack &trk);
    void cmdPlayMemacc(MP2KTrack &trk);
    void cmdPlayXCmd(MP2KTrack &trk);
};
