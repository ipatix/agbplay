#include "MP2KContext.h"
#include "ConfigManager.h"

MP2KContext::MP2KContext(const Rom &rom, const MP2KSoundMode &mp2kSoundMode, const AgbplaySoundMode &agbplaySoundMode)
    : rom(rom), reader(*this), mixer(*this, STREAM_SAMPLERATE, 1.0f), player(agbplaySoundMode.trackLimit, 0), mp2kSoundMode(mp2kSoundMode), agbplaySoundMode(agbplaySoundMode), memaccArea(256)
{
}

void MP2KContext::SoundMain()
{
    reader.Process();
    mixer.Process();
    curInterFrame++;
}

void MP2KContext::InitSong(size_t songHeaderPos)
{
    sndChannels.clear();
    sq1Channels.clear();
    sq2Channels.clear();
    waveChannels.clear();
    noiseChannels.clear();

    curInterFrame = 0;
    player.Init(songHeaderPos);
    reader.Restart();
    mixer.ResetFade();

    uint32_t fixedModeRate = reader.freqLut.at(mp2kSoundMode.freq - 1);
    uint8_t reverb = 0;
    if (player.GetReverb() & 0x80)
        reverb = player.GetReverb() & 0x7F;
    else if (mp2kSoundMode.rev & 0x80)
        reverb = mp2kSoundMode.rev & 0x7F;
    float pcmMasterVolume = static_cast<float>(mp2kSoundMode.vol + 1) / 16.0f;
    uint8_t numTracks = static_cast<uint8_t>(player.tracks.size());

    mixer.Init(fixedModeRate, reverb, pcmMasterVolume, agbplaySoundMode.reverbType);
}

bool MP2KContext::HasEnded() const
{
    return reader.EndReached() && mixer.IsFadeDone();
}

size_t MP2KContext::GetCurInterFrame() const
{
    return curInterFrame;
}
