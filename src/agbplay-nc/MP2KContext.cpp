#include "MP2KContext.h"
#include "ConfigManager.h"

MP2KContext::MP2KContext(const Rom &rom, const MP2KSoundMode &mp2kSoundMode, const AgbplaySoundMode &agbplaySoundMode, const SongTableInfo &songTableInfo)
    : rom(rom), reader(*this), mixer(*this, STREAM_SAMPLERATE, 1.0f), player(agbplaySoundMode.trackLimit, 0), mp2kSoundMode(mp2kSoundMode), agbplaySoundMode(agbplaySoundMode), songTableInfo(songTableInfo), memaccArea(256)
{
}

void MP2KContext::m4aSoundMain()
{
    reader.Process();
    mixer.Process();
    curInterFrame++;
}

void MP2KContext::m4aSongNumStart(uint16_t songId)
{
    if (songId >= songTableInfo.songCount)
        throw Xcept("Failed to load out of range songId={} (total songCount={})", songId, songTableInfo.songCount);
    const size_t songPos = rom.ReadAgbPtrToPos(songTableInfo.songTablePos + songId * 8 + 0);
    const uint8_t playerIdx = rom.ReadU8(songTableInfo.songTablePos + songId * 8 + 4);
    m4aMPlayStart(playerIdx, songPos);
}

void MP2KContext::m4aMPlayStart(uint8_t playerIdx, size_t songPos)
{
    // TODO implement player number usage
    curInterFrame = 0;
    player.Init(songPos);
    reader.Restart();
    mixer.ResetFade();

    uint32_t fixedModeRate = reader.freqLut.at(mp2kSoundMode.freq - 1);
    uint8_t reverb = 0;
    if (player.GetReverb() & 0x80)
        reverb = player.GetReverb() & 0x7F;
    else if (mp2kSoundMode.rev & 0x80)
        reverb = mp2kSoundMode.rev & 0x7F;
    float pcmMasterVolume = static_cast<float>(mp2kSoundMode.vol + 1) / 16.0f;

    mixer.Init(fixedModeRate, reverb, pcmMasterVolume, agbplaySoundMode.reverbType);
}

void MP2KContext::SoundClear()
{
    sndChannels.clear();
    sq1Channels.clear();
    sq2Channels.clear();
    waveChannels.clear();
    noiseChannels.clear();
}

bool MP2KContext::HasEnded() const
{
    return reader.EndReached() && mixer.IsFadeDone();
}

size_t MP2KContext::GetCurInterFrame() const
{
    return curInterFrame;
}
