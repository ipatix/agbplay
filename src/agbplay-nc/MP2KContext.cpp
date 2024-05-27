#include "MP2KContext.h"
#include "ConfigManager.h"

MP2KContext::MP2KContext(const Rom &rom, const MP2KSoundMode &soundMode, const AgbplayMixingOptions &mixingOptions)
    : rom(rom), reader(*this), mixer(*this, STREAM_SAMPLERATE, 1.0f), player(mixingOptions.trackLimit, 0), soundMode(soundMode), mixingOptions(mixingOptions), memaccArea(256)
{
}

void MP2KContext::Process(std::vector<std::vector<sample>>& trackAudio)
{
    reader.Process();
    mixer.Process(trackAudio);
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

    uint32_t fixedModeRate = reader.freqLut.at(soundMode.freq - 1);
    uint8_t reverb = 0;
    if (player.GetReverb() & 0x80)
        reverb = player.GetReverb() & 0x7F;
    else if (soundMode.rev & 0x80)
        reverb = soundMode.rev & 0x7F;
    float pcmMasterVolume = static_cast<float>(soundMode.vol + 1) / 16.0f;
    uint8_t numTracks = static_cast<uint8_t>(player.tracks.size());

    mixer.Init(fixedModeRate, reverb, pcmMasterVolume, mixingOptions.reverbType, numTracks);
}

bool MP2KContext::HasEnded() const
{
    return reader.EndReached() && mixer.IsFadeDone();
}

size_t MP2KContext::GetCurInterFrame() const
{
    return curInterFrame;
}
