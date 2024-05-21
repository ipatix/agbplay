#pragma once

#include <vector>

#include "SequenceReader.h"
#include "SoundMixer.h"
#include "Rom.h"

/* Instead of defining lots of global objects, we define
 * a context with all the things we need. So anything which
 * needs anything out of this context only needs a reference
 * to a PlayerContext */

struct PlayerContext {
    PlayerContext(const Rom &rom, const MP2KSoundMode &soundMode, const AgbplayMixingOptions &mixingOptions);
    PlayerContext(const PlayerContext&) = delete;
    PlayerContext& operator=(const PlayerContext&) = delete;

    void Process(std::vector<std::vector<sample>>& trackAudio);
    void InitSong(size_t songPos);
    bool HasEnded() const;
    size_t GetCurInterFrame() const;

    const Rom& rom;
    SequenceReader reader;
    SoundMixer mixer;
    Sequence seq;
    MP2KSoundMode soundMode;
    AgbplayMixingOptions mixingOptions;

    // sound channels
    std::list<SoundChannel> sndChannels;
    std::list<SquareChannel> sq1Channels;
    std::list<SquareChannel> sq2Channels;
    std::list<WaveChannel> waveChannels;
    std::list<NoiseChannel> noiseChannels;

    size_t curInterFrame = 0;
};
