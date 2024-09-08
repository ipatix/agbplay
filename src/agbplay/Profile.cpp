#include "Profile.hpp"

void Profile::ApplyScanToPlayback(
    const SongTableInfo &songTableInfoScan,
    const PlayerTableInfo &playerTableScan,
    const MP2KSoundMode &mp2kSoundModeScan
)
{
    /* SongTableInfo */
    songTableInfoPlayback = songTableInfoConfig;
    if (songTableInfoConfig.pos == SongTableInfo::POS_AUTO)
        songTableInfoPlayback.pos = songTableInfoScan.pos;
    if (songTableInfoConfig.count == SongTableInfo::COUNT_AUTO)
        songTableInfoPlayback.count = songTableInfoScan.count;

    /* PlayerTableInfo */
    playerTablePlayback.clear();
    if (playerTableConfig.size() == 0) {
        playerTablePlayback = playerTableScan;
    } else {
        for (auto &p : playerTableConfig)
            playerTablePlayback.emplace_back(p);
    }

    /* MP2KSoundMode */
    mp2kSoundModePlayback = mp2kSoundModeConfig;
    if (mp2kSoundModeConfig.vol == MP2KSoundMode::VOL_AUTO)
        mp2kSoundModePlayback.vol = mp2kSoundModeScan.vol;
    if (mp2kSoundModeConfig.rev == MP2KSoundMode::REV_AUTO)
        mp2kSoundModePlayback.rev = mp2kSoundModeScan.rev;
    if (mp2kSoundModeConfig.freq == MP2KSoundMode::FREQ_AUTO)
        mp2kSoundModePlayback.freq = mp2kSoundModeScan.freq;
    if (mp2kSoundModeConfig.maxChannels == MP2KSoundMode::CHN_AUTO)
        mp2kSoundModePlayback.maxChannels = mp2kSoundModeScan.maxChannels;
    if (mp2kSoundModeConfig.dacConfig == MP2KSoundMode::DAC_AUTO)
        mp2kSoundModePlayback.dacConfig = mp2kSoundModeScan.dacConfig;
}
