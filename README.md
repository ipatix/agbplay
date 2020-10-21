## agbplay
__agbplay__ is a music player with Terminal interface for GBA ROMs that use
the most common (mp2k/m4a) sound engine format. The code itself is written in
C++.

### Quick overview
![agbplay](https://user-images.githubusercontent.com/8502545/95079441-e9e97c00-0716-11eb-8ea2-5240a19614ae.png)


### Controls
- Arrow Keys or HJKL: Navigate through the program
- Tab: Change between playlist and songlist
- A: Add the selected song to the playlist
- D: Delete the selected song from the playlist
- T: Toggle whether the song should be output to a file (see R and E)
- G: Drag the song through the playlist for ordering
- I: Force song restart
- O: Song play/pause
- P: Force song stop
- +=: Double the playback speed
- -: Halve the playback speed
- Enter: Toggle track muting
- M: Mute selected track
- S: Solo selected track
- U: Unmute all tracks
- N: Rename the selected song in the playlisy
- E: Export selected songs to individual track files (to "$cwd/wav")
- R: Export selected songs to files (non-split)
- B: Benchmark, run the export program but don't write to file
- F: Save Playlist: The playlist is also saved when the program is closed
- Q or Ctrl-D: Exit rrogram

### Current state of things
- ROMs can be loaded and scanned for the songtable automatically
- PCM playback works pretty much perfectly; GB instruments sound great, but
  envelope curves are not 100% accurate
- Basic rendering to file done, including dummy writing for benchmarking

### To do
- Add missing key explanation for controls
- Change to an audio library that doesn't print ANYTHING messages on stdout

### Dependencies (Debian / Arch)

- `libboost-all-dev` / `boost`
- `portaudio19-dev` / `portaudio`
- `libncursesw5-dev` / `ncurses5-compat-libs` <sup>AUR</sup>
- `libsndfile1-dev` / `libsndfile`
- `libjsoncpp-dev` / `jsoncpp`

### Configuration JSON
Since 21.10.2020, agbplay uses a standard JSON format for storing playlists and
other configuration data.

Take a look at this sample scheme:
```
{
    "id" : "agbplay",
    "playlists" : 
    [
        {
            "games" : 
            [
                "BPED", "BPEE"
            ],
            "pcm-fixed-rate-resampling-algo" : "blep",
            "pcm-master-volume" : 12,
            "pcm-resampling-algo" : "linear",
            "pcm-reverb-buffer-len" : 1584,
            "pcm-reverb-level" : 0,
            "pcm-reverb-type" : "normal",
            "pcm-samplerate" : 4,
            "song-track-limit" : 10,
            "songs" : 
            [
                {
                    "index" : 414,
                    "name" : "Intro Video"
                },
                {
                    "index" : 442,
                    "name" : "The Pokemon"
                },
                {
                    "index" : 413,
                    "name" : "Title Screen"
                },
            ]
        },
        {
            "games" :
            [
                "AGSE"
            ],
            ...
        }
    ]
```

The root element in the JSON only has two properties: `id` and `playlists`.
`id` is a fixed string always set to `"agbplay"`. `playlists` is the array what
contains the playlist data for the games.

Each element in the array contains the following properties:

- `pcm-master-volume`: Value from 0 to 15.
  The correct setting for this value depends on the game and has to be reverse engineered individually.
- `pcm-samplerate`: Value from 0 to 15.
  The correct setting for this value depends on the game and has to be reverse engineered individually.
- `pcm-reverb-level`: I have not seen any games which use this.
  Can be used to apply reverb even for songs that don't use it. Set to 0 if you don't care.
- `pcm-reverb-buffer-len`: This is fixed to 1536 in Nintendo's engine, but if you want to experiement with reverb, you can change this.
- `pcm-reverb-type`: Different games use different reverb implementations. Check `Sound formatting notes` below for details.
- `pcm-resampling-algo` and `pcm-fixed-rate-resampling-algo`
- `song-track-limit`: Limit the number of tracks the engine can play.
  Useful for games which have an engine limit, but the song contain more tracks than the engine can handle.

#### Sound formatting notes

On Nintendo's engine (that runs on the hardware) it allows the developer to set
a master volume for PCM sound from 0 to 15. This doesn't affect CGB sounds and
changing it will result in a different volume ratio between PCM and CGB sounds.

As for the reverb level, you can globally set it from 0 to 127. This overrides
the song's reverb settings in their song header.

The 'magic' samplerate values are listed below. Note that the 'magic' values
orrespond to the values like they are used by m4aSoundMode (values: 1-12). `agbplay` will use
this 'magic' value to get the sample rate for so-called "fixed frequency
sounds".

Magic values (in Hz): `5734`, `7884`, `10512`, `13379`, `15768`, `18157`,
`21024`, `26758`, `31536`, `36314`, `40137`, `42048`

One more thing about reverb: Most games just use Nintendo's default reverb algorithm (or reverb of 0 for no reverb at all). However, some games have implemented their own algorithms. You can use the following values in combination with the option `pcm-reverb-type` to set it:

- `normal` = Nintendo's normal reverb algorithm
- `gs1` = Camelot's reverb used in Golden Sun 1
- `gs2` = Camelot's reverb used in Golden Sun TLA (aka Golden Sun 2)
- `mgat` = Camelot's reverb used in Mario Golf - Advance Tour
- `test` = Only use this if you use the TestReverb class for developing your own algrithm
- `none` = disabled (not used in normal games)

Last but not least, agbplay now supports different resampling algorithms which
can be set in the JSON-File. There is a setting for normal PCM sounds
`pcm-resampling-algo` and `pcm-fixed-rate-resampling-algo` for fixed frequency sounds (mostly used
for drums). They sypport the following values:

- `nearest` = Fast! Commonly referred to as "no interpolation". Sounds pretty
  bad in most cases but can give you that low quality crunchyness. You most
  likely want to use `blep` over this one (`nearest` is wayyyyyyy cheaper to
  compute, though).
- `linear` = Fast! Interpolate samples in a triangular fasion. This is what's
  used with Nintendo's sound driver (although with different target samplerates).
  Recommended for normal sounds.
- `sinc` = Slow! Use a sinc based filter to avoid aliasing. For most games this
  will filter out a lot of the high end freuqnecies. The only case I'd
  recommend this is for games that generally use high samplerate waveforms (I
  like to use it on Golden Sun TLA which uses 31 kHz for drums).
- `blep` = Slow! This generates bandlimited rectangular pulses for the samples.
  It's similar to `nearest` but `nearest` will not bandlimit the rectangular pulses
  so it's going to cause frequency band folding. Use `blep` if you want to fake
  some brightness into your drums (i.e. fixed frequency sounds) since this is
  the way hardware does it (except `blep` will clean up the higher frequencies
  which `nearest` doesn't).

### Additional information

#### Debian portaudio issues

If you have issues installing portaudio19-dev on Debian (conflicting packages) make sure to install "libjack-jackd2-dev" before. The reason for this is that portaudio on Debian depends on either the old dev package for jack or the jack2 dev package. By default apt wants to install the old one which for some reason causes problems on a lot of systems.

#### Building

Install all dependencies (listed above) and run `make`.

The code itself is written to be cross-platform. That's why I've decided to go
with Boost and portaudio.

It has been tested on Cygwin (Windows), Debian and Arch Linux, all on x86-64.
Native Windows support with Visual Studio is NOT supported by me and I NEVER
will. Getting terminal things to work on Windows with UTF-8, colors and
resizing terminal just doesn't work.

#### Contributing

If you have any suggestions feel free to open up a pull request or just an
issue with some basic information. For issues I'm mostly focused on fixing bugs
and not really on any new features.

Please be reminded that this was a "C++ learning project" for me and therefore
the code is quite weird and probably contains a lot of "bad practices" in a few
places.
