#include "Profile.hpp"

std::atomic<uint32_t> Profile::sessionIdCounter = 1;

/* We define custom copy and assignment constructors.
 * Because each profile has a unique ID, we cannot simply perform a member-wise copy. */

Profile::Profile(const Profile &rhs)
{
    *this = rhs;
}

Profile &Profile::operator=(const Profile &rhs)
{
    playlist = rhs.playlist;
    songTableInfoConfig = rhs.songTableInfoConfig;
    songTableInfoScanned = rhs.songTableInfoScanned;
    songTableInfoPlayback = rhs.songTableInfoPlayback;
    playerTableConfig = rhs.playerTableConfig;
    playerTableScanned = rhs.playerTableScanned;
    playerTablePlayback = rhs.playerTablePlayback;
    mp2kSoundModeConfig = rhs.mp2kSoundModeConfig;
    mp2kSoundModeScanned = rhs.mp2kSoundModeScanned;
    mp2kSoundModePlayback = rhs.mp2kSoundModePlayback;
    agbplaySoundMode = rhs.agbplaySoundMode;
    gameMatch = rhs.gameMatch;
    name = rhs.name;
    author = rhs.author;
    gameStudio = rhs.gameStudio;
    description = rhs.description;
    notes = rhs.notes;
    dirty = true;
    return *this;
}

void Profile::ApplyScanToPlayback()
{
    /* SongTableInfo */
    songTableInfoPlayback = songTableInfoConfig;
    if (songTableInfoConfig.pos == SongTableInfo::POS_AUTO)
        songTableInfoPlayback.pos = songTableInfoScanned.pos;
    if (songTableInfoConfig.count == SongTableInfo::COUNT_AUTO)
        songTableInfoPlayback.count = songTableInfoScanned.count;

    /* PlayerTableInfo */
    playerTablePlayback.clear();
    if (playerTableConfig.size() == 0) {
        playerTablePlayback = playerTableScanned;
    } else {
        for (auto &p : playerTableConfig)
            playerTablePlayback.emplace_back(p);
    }

    /* MP2KSoundMode */
    mp2kSoundModePlayback = mp2kSoundModeConfig;
    if (mp2kSoundModeConfig.vol == MP2KSoundMode::VOL_AUTO)
        mp2kSoundModePlayback.vol = mp2kSoundModeScanned.vol;
    if (mp2kSoundModeConfig.rev == MP2KSoundMode::REV_AUTO)
        mp2kSoundModePlayback.rev = mp2kSoundModeScanned.rev;
    if (mp2kSoundModeConfig.freq == MP2KSoundMode::FREQ_AUTO)
        mp2kSoundModePlayback.freq = mp2kSoundModeScanned.freq;
    if (mp2kSoundModeConfig.maxChannels == MP2KSoundMode::CHN_AUTO)
        mp2kSoundModePlayback.maxChannels = mp2kSoundModeScanned.maxChannels;
    if (mp2kSoundModeConfig.dacConfig == MP2KSoundMode::DAC_AUTO)
        mp2kSoundModePlayback.dacConfig = mp2kSoundModeScanned.dacConfig;
}
