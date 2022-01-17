#pragma once

#include <vector>

#include "Rom.h"
#include "SequenceReader.h"
#include "SoundMixer.h"
#include "SoundData.h"

/* Instead of defining lots of global objects, we define
 * a context with all the things we need. So anything which
 * needs anything out of this context only needs a reference
 * to a PlayerContext */

struct PlayerContext {
    PlayerContext(Rom& rom, int8_t maxLoops, uint8_t maxTracks, EnginePars pars);

    void Process(std::vector<std::vector<sample>>& trackAudio);
    void InitSong(size_t songPos);
    bool HasEnded() const;

    Rom rom;
    SequenceReader reader;
    SoundMixer mixer;
    Sequence seq;
    SoundBank bnk;
    EnginePars pars;

    // sound channels
    std::list<SoundChannel> sndChannels;
    std::list<SquareChannel> sq1Channels;
    std::list<SquareChannel> sq2Channels;
    std::list<WaveChannel> waveChannels;
    std::list<NoiseChannel> noiseChannels;
};
