# agbplay
"agbplay" is a music player with Terminal interface for GBA ROMs that use the most common (aka mp2k/m4a) sound engine format.
The code itself is written in C++.

### Quick overview:
![agbplay](https://cloud.githubusercontent.com/assets/8502545/24632845/faa2503c-18c5-11e7-84a3-cecec08e034a.png)

### Controls:
- Arrow Keys or HJKL: Navigate through the program
- Tab: Change between Playlist and Songlist
- A: Add the selected song to the playlist
- D: Delete the selected song from the playlist
- T: Toggle whether the song should be output to a file (see R and E)
- G: Drag the song through the playlist for ordering
- I: Force Song Restart
- O: Song Play/Pause
- P: Force Song Stop
- +=: Double the playback speed
- -: Halve the playback speed
- Enter: Toggle Track Muting
- M: Mute selected Track
- S: Solo selected Track
- U: Unmute all Tracks
- N: Rename the selected song in the playlisy
- E: Export selected songs to individual track files (to "workdirectory/wav")
- R: Export selected songs to files (non-split)
- B: Benchmark, Run the export program but don't write to file
- Q or Ctrl-D: Exit Program

### Current state of things:
- ROMs can be loaded and scanned for the songtable automatically
- PCM playback works pretty much perfect, GB instruments sound great, but envelope curves are not 100% accurate
- Basic rendering to file done including dummy writing for benchmarking

### Todo:
- Add missing key explanation for controls
- Change to an audio library that doesn't print ANYTHING messages on stdout
- Redo config system (perhaps in XML) to support games to share playlist (for games only differing in terms of language) and to allow multiple playlists for one game

### Depenencies (Debian Package Names):
- libboost-all-dev
- portaudio19-dev
- libncursesw5-dev
- libsndfile1-dev

### INI-File:
The INI file is intended to be self explanatory. Each game has a section which starts with the game's code from the ROM-header:
```
[GAME]
```
The song name's are stored with a 4 decimal digit key (which corresponds to the song's ID) assigned to a value with the song's name:
```
[GAME]
0001 = Main Theme
0002 = Boss Battle A
0015 = Credits Music
```
Usually, you don't have to put in the song names in there manually since you can add them through the Terminal interface.

In addition to that you can set the game's engine runtime parameters (reverb, samplerate, master volume). The following example set's the game's master volume to 14, default reverb to 0 (i.e. off) and a sample rate setting of 6:
```
[GAME]
ENG_VOL = 14
ENG_REV = 0
ENG_FREQ = 6
0001 = Main Theme
0002 = Boss Battle A
0015 = Credits Music
```

But what do those magic values mean?
On Nintendo's engine (that runs on the hardware) it allows the developer to set a master volume for PCM sound from 0 to 15. This doesn't affect CGB sounds and changing it will result in a different volume ratio between PCM and CGB sounds.
As for the reverb level, you can globally set it from 0 to 127. This overrides the song's reverb settings in their song header.
The 'magic' samplerate values are shown in the following table. Note that the 'magic' values correspond to the values like they are used by m4aSoundMode. agbplay will use this 'magic' value to get the sample rate for so-called "fixed frequency sounds".

1. __5734 Hz__
2. __7884 Hz__
3. __10512 Hz__
4. __13379 Hz__
5. __15768 Hz__
6. __18157 Hz__
7. __21024 Hz__
8. __26758 Hz__
9. __31536 Hz__
10. __36314 Hz__
11. __40137 Hz__
12. __42048 Hz__

One more thing about reverb: Most games just use Nintendo's default reverb algorithm (or reverb of 0 for no reverb at all). However, some games have implemented their own algorithms. You can use the following values in combination with the option `ENG_REV_TYPE` to set it:

- `NORMAL` = Nintendo's normal reverb algorithm
- `GS1` = Camelot's reverb used in Golden Sun 1
- `GS2` = Camelot's reverb used in Golden Sun TLA (aka Golden Sun 2)
- `MGAT` = Camelot's reverb used in Mario Golf - Advance Tour
- `TEST` = Only use this if you use the TestReverb class for developing your own algrithm
- `NONE` = disabled (not used in normal games)


Last but not least, agbplay now supports different resampling algorithms which can be set in the INI-File. There is a setting for normal PCM sounds `PCM_RES_TYPE` and `PCM_FIX_RES_TYPE` for fixed frequency sounds (mostly used for drums).
They sypport the following values:

- `NEAREST` = Fast! Commonly referred to as "no interpolation". Sounds pretty bad in most cases but can give you that low quality crunchyness. You most likely want to use BLEP over this one (NEAREST is wayyyyyyy cheaper to compute, though).
- `LINEAR` = Fast! Interpolate samples in a triangular fasion. This is what's used on Nintendo's hardware (although with different target samplerates). Recommended for normal sounds.
- `SINC` = Slow! Use a sinc based filter to avoid aliasing. For most games this will filter out a lot of the high end freuqnecies. The only case I'd recommend this is for games that generally use high samplerate waveforms (I like to use it on Golden Sun TLA which uses 31 kHz for drums).
- `BLEP` = Slow! This generates bandlimited rectangular pulses for the samples. It's similar to NEAREST but NEAREST will not bandlimit the rectangular pulses, so it's going to cause frequency band folding. Use BLEP if you want to fake some brightness into your drums (i.e. fixed frequency sounds) since this is the way hardware does it (except BLEP will clean up the higher frequencies which NEAREST doesn't).

This is how a finaly config would look like:
```
[GAME]
ENG_VOL = 14
ENG_REV = 0
ENG_REV_TYPE = NORMAL
ENG_FREQ = 6
PCM_RES_TYPE = LINEAR
PCM_FIX_RES_TYPE = BLEP
TRACK_LIMIT = 12
0001 = Main Theme
0002 = Boss Battle A
0015 = Credits Music
```

`TRACK_LIMIT` will simply limit the amount of tracks a song can use. This is useful to accurately playback games which try to use more tracks than are available on the specific hardware configuration.

### Other Information

If you have issues installing portaudio19-dev on Debian (conflicting packages) make sure to install "libjack-jackd2-dev" before.
The reason for this is that portaudio on Debian depends on either the old dev package for jack or the jack2 dev package. By default apt wants to install the old one which for some reason causes problems on a lot of systems.

Compiling:
Install all the required dev packages and run the Makefile (make).

The code itself is written to be cross-platform compatible. That's why I've decided to go with Boost and portaudio.
I personally have tested it on 64 bit Cygwin (Windows) and 64 bit Debian Linux.
Native Windows support with Visual Studio is NOT supported by me and I NEVER will. Getting terminal things to work on Windows with UTF-8, colors and resizing terminal just doesn't work.

Contibuting:
If you have any suggestions feel free to open up a Pull Request or just an Issue with some basic information. For issues I'm at the momentan mostly focused on fixing bugs and not really on any new features.
Please be reminded that this was a "C++ learning project" for me and therefore the code is quite weird and probably contains a lot of "bad practices" in a few places.
