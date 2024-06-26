#include "MP2KContext.h"

#include <algorithm>
#include <cassert>

#include "ConfigManager.h"

MP2KContext::MP2KContext(const Rom &rom, const MP2KSoundMode &mp2kSoundMode, const AgbplaySoundMode &agbplaySoundMode, const SongTableInfo &songTableInfo, const PlayerTableInfo &playerTableInfo)
    : rom(rom), reader(*this), mixer(*this, STREAM_SAMPLERATE, 1.0f), mp2kSoundMode(mp2kSoundMode), agbplaySoundMode(agbplaySoundMode), songTableInfo(songTableInfo), memaccArea(256)
{
    assert(playerTableInfo.size() <= 32);

    for (size_t i = 0; i < playerTableInfo.size(); i++)
        players.emplace_back(playerTableInfo.at(i), static_cast<uint8_t>(i));
}

void MP2KContext::m4aSoundMain()
{
    reader.Process();
    mixer.Process();
    curInterFrame++;
}

void MP2KContext::m4aSongNumStart(uint16_t songId)
{
    if (songId >= songTableInfo.count)
        throw Xcept("Failed to load out of range songId={} (total count={})", songId, songTableInfo.count);
    const size_t songPos = rom.ReadAgbPtrToPos(songTableInfo.pos + songId * 8 + 0);
    const uint8_t playerIdx = m4aSongNumPlayerGet(songId);
    primaryPlayer = playerIdx;
    m4aMPlayStart(playerIdx, songPos);
}

void MP2KContext::m4aSongNumStop(uint16_t songId)
{
    if (songId >= songTableInfo.count)
        throw Xcept("Failed to load out of range songId={} (total count={})", songId, songTableInfo.count);
    m4aMPlayStop(m4aSongNumPlayerGet(songId));
}

void MP2KContext::m4aMPlayStart(uint8_t playerIdx, size_t songPos)
{
    MP2KPlayer &player = players.at(playerIdx);

    for (MP2KTrack &trk : player.tracks)
        trk.Stop();

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

    // TODO we should not need to init all mixer parameters, only reverb?
    mixer.Init(fixedModeRate, reverb, pcmMasterVolume, agbplaySoundMode.reverbType);
}

void MP2KContext::m4aMPlayStop(uint8_t playerIdx)
{
    MP2KPlayer &player = players.at(playerIdx);

    player.enabled = false;

    for (MP2KTrack &trk : player.tracks)
        trk.Stop();
}

void MP2KContext::m4aMPlayAllStop()
{
    for (size_t i = 0; i < players.size(); i++)
        m4aMPlayStop(static_cast<uint8_t>(i));
}

void MP2KContext::m4aMPlayContinue(uint8_t playerIdx)
{
    players.at(playerIdx).enabled = true;
}

void MP2KContext::m4aSoundClear()
{
    sndChannels.clear();
    sq1Channels.clear();
    sq2Channels.clear();
    waveChannels.clear();
    noiseChannels.clear();
}

uint8_t MP2KContext::m4aSongNumPlayerGet(uint16_t songId) const
{
    return rom.ReadU8(songTableInfo.pos + songId * 8 + 4);
}

bool MP2KContext::m4aMPlayIsPlaying(uint8_t playerIdx) const
{
    return players.at(playerIdx).enabled;
}

bool MP2KContext::HasEnded() const
{
    return reader.EndReached() && mixer.IsFadeDone();
}

size_t MP2KContext::GetCurInterFrame() const
{
    return curInterFrame;
}

void MP2KContext::GetVisualizerState(MP2KVisualizerState &visualizerState)
{
    visualizerState.activeChannels = sndChannels.size();
    visualizerState.players.resize(players.size());
    visualizerState.primaryPlayer = primaryPlayer;

    for (size_t playerIdx = 0; playerIdx < players.size(); playerIdx++) {
        MP2KPlayer &player_src = players.at(playerIdx);
        auto &player_dst = visualizerState.players.at(playerIdx);

        player_dst.tracks.resize(player_src.tracks.size());
        player_dst.tracksUsed = player_src.tracksUsed;
        player_dst.bpm = player_src.bpm;
        player_dst.bpmFactor = reader.GetSpeedFactor();

        for (size_t trackIdx = 0; trackIdx < player_src.tracks.size(); trackIdx++) {
            MP2KTrack &trk_src = player_src.tracks.at(trackIdx);
            auto &trk_dst = player_dst.tracks.at(trackIdx);

            trk_src.loudnessCalculator.CalcLoudness(trk_src.audioBuffer.data(), trk_src.audioBuffer.size());
            float volLeft, volRight;
            trk_src.loudnessCalculator.GetLoudness(volLeft, volRight);

            trk_dst.trackPtr = static_cast<uint32_t>(trk_src.pos);
            trk_dst.isCalling = trk_src.patternLevel > 0;
            trk_dst.isMuted = trk_src.muted;
            trk_dst.vol = trk_src.vol;
            trk_dst.mod = trk_src.mod;
            trk_dst.prog = trk_src.prog;
            trk_dst.pan = trk_src.pan;
            trk_dst.pitch = trk_src.pitch;
            trk_dst.envL = uint8_t(std::clamp<uint32_t>(uint32_t(volLeft * 768.f), 0, 255));
            trk_dst.envR = uint8_t(std::clamp<uint32_t>(uint32_t(volRight * 768.f), 0, 255));
            trk_dst.delay = std::max<uint8_t>(0, static_cast<uint8_t>(trk_src.delay));
            trk_dst.activeNotes = trk_src.activeNotes;
        }
    }

    masterLoudnessCalculator.CalcLoudness(masterAudioBuffer.data(), masterAudioBuffer.size());
    masterLoudnessCalculator.GetLoudness(visualizerState.masterVolLeft, visualizerState.masterVolRight);
}
