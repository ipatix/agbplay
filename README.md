## agbplay with Live MIDI mode
This is a fork of __agbplay__ that adds a Live MIDI mode for playing the synth directly with MIDI input.

__agbplay__ is a music player with Terminal interface for GBA ROMs that use
the most common (mp2k/m4a) sound engine format. The code itself is written in
C++.

### Quick overview
![agbplay](https://user-images.githubusercontent.com/130002142/246687168-70a6d76b-575b-4e8c-9ccd-713f9a9db1c3.png)

### Usage

```
agbplay <ROM.gba> [song table position] [MIDI port number]
```
- **ROM.gba**: Path to ROM for playback.
- **song table position**: Song table position in hex (e.g., `55E0` or `0x55E0`). Specify `0` if you want to leave unset.
- **MIDI port number**: MIDI port number to use. If provided and the port is valid, Live MIDI mode will be enabled.

### Live MIDI mode

To list MIDI port numbers that can be used as the MIDI port number value, use `agbplay --help`.

```
There are 9 MIDI input sources available.
  Input Port #1: IAC Driver Bus 1
  Input Port #2: IAC Driver Bus 2
  Input Port #3: CASIO USB-MIDI
```

When Live MIDI mode is enabled, normal song playback is disabled. Instead, the song is immediately loaded with its instrument set and all control is given to MIDI input.

#### MIDI reference

* MIDI channel corresponds to track number (1-16).
* Program change does as expected, however note that no sound will be produced until a program change is issued.
* Pitch bend works as expected, however the resolution is limited to the upper 7-bits (-64 to 63). Bend range can be set with the BENDR control.
* Control change
  * **1**: Modulation depth (see settings below)
  * **7**: Volume
  * **10**: Pan 
  * **20**: Pitch bend range
  * **21**: Modulation speed
  * **22**: Modulation type
    * **0**: Vibrato
    * **1**: Tremolo
    * **2**: Auto pan
  * **24**: Fine tune
  * **26**: Modulation delay (ticks before mod kicks in)
  * **33**: Polyphony note priority
  * **123**: All notes off
* Issuing extended commands like psuedo-echo aren't supported yet in Live MIDI mode.

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
- !: Show extended song information

### Current state of things
- ROMs can be loaded and scanned for the songtable automatically
- PCM playback works pretty much perfectly; GB instruments sound great, but
  envelope curves are not 100% accurate
- Basic rendering to file done, including dummy writing for benchmarking

### To do
- Add missing key explanation for controls
- Change to an audio library that doesn't print ANYTHING messages on stdout

### Dependencies

Homebrew | Debian | Arch | Cygwin
--- | --- | --- | ---
Xcode CLI tools | `build-essential` | `base-devel` | `make`, `gcc-g++`
`boost` | `libboost-all-dev` | `boost` | `libboost-devel`
`portaudio` | `portaudio19-dev` | `portaudio` | `libportaudio-devel`
`ncurses` | `libncursesw5-dev` | `ncurses5-compat-libs` <sup>AUR</sup> | `libncurses-devel`
`libsndfile` | `libsndfile1-dev` | `libsndfile` | `libsndfile-devel`
`jsoncpp` | `libjsoncpp-dev` | `jsoncpp` | `libjsoncpp-devel`
`rtmidi` | `librtmidi-dev` | `rtmidi` | `librtmidi-devel`

### Configuration JSON
Since 21.10.2020, agbplay uses a standard JSON format for storing playlists and
other configuration data.

Take a look at this sample scheme:
```
{
    "id" : "agbplay",
    "cgb-polyphony" : "mono-strict",
    "wave-output-dir" : "/home/misterx/Music/agbplay",
    "max-loops-export" : 1,
    "max-loops-playlist" : 1,
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
}
```

The root element in the JSON has the following properties:
- `id` is a fixed string and should always be set to `agbplay`.
- `playlists` contains the array of the actual tagged songs. See below for details.
- `wave-output-dir` specifies the directory which is used for exporting songs from agbplay.
- `cgb-polyphony` specifies whether CGB sounds should be allowed to be polyphonic. Valid values are `mono-strict`, `mono-smooth`, `poly`.
- `max-loops-export` specifies how many times songs should loop before fading out when exporting to a file.
- `max-loops-playlist` specifies how many times songs should loop before fading out when listening to a song within the program. This value can be set to `-1` to make songs loop endlessly.

Each playlist entry in the array contains the following properties:

- `games`: A list of game codes which should use the specified playlist.
  agbplay doesn't generate this on its own, but you can manually edit the JSON
  to let games share a playlist (for example different localizations).
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
- `songs`: This is again an array which contains all the playlist's songs.
  Format is pretty straight forward. There is an `index` property and a `name` property for each song.

#### Sound formatting notes

On Nintendo's engine (that runs on the hardware) it allows the developer to set
a master volume for PCM sound from 0 to 15. This doesn't affect CGB sounds and
changing it will result in a different volume ratio between PCM and CGB sounds.

As for the reverb level, you can globally set it from 0 to 127. This overrides
the song's reverb settings in their song header.

The 'magic' samplerate values are listed below. Note that the 'magic' values
correspond to the values like they are used by m4aSoundMode (values: 1-12). `agbplay` will use
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
- `blamp` = Slow! Same as blep but creates bandlimited triangular pulses instead
  of rectangular ones. Use this as high quality alternative to `linear`.

#### Importing tags from GSF files

Manually creating playlists/tags for some games can be avoided if you can find
an existing GSF set for that particular game.

Use the supplied `playlist_from_gsf.py` script and pass it a set of `.minigsf` files.
The script will then parse the song names and song numbers from those files and will
output a JSON formatted array that then can be used for the property `songs`, which
is explained above. So you can copy that JSON array into your `agbplay.json` for
that particular game.

### Additional information

#### Debian portaudio issues

If you have issues installing portaudio19-dev on Debian (conflicting packages) make sure to install "libjack-jackd2-dev" before. The reason for this is that portaudio on Debian depends on either the old dev package for jack or the jack2 dev package. By default apt wants to install the old one which for some reason causes problems on a lot of systems.

#### "Missing DLLs"

If you happen to get errors about missing DLLs and you compiled agbplay under
the Cygwin environement, you also have to run agbplay from the Cygwin environment.
Cygwin compiled software does require the Cygwin runtime for 99% of the programs,
so please accept that you have to do this for agbplay as well.

#### Terminal Colors

agbplay requires 256 color terminal support. If you happen to see the message
`Terminal does not support 256 colors`, you may have to use a different
terminal emulator or you have to fix your `TERM` variable.

If you are using the Cygwin environment, you can do the following:

- Right click on the titlebar of the Cygwin Terminal
- Click Options
- Select "Terminal" in the tree view
- Change Type to `xterm-256color`

Another option is to use the Windows Terminal from the Windows Store
(although it sometimes still seems to have a few graphical issues).

**Never ever** set your `TERM` variable in your `.bashrc` or equivalent. This will
cause issues if you are running your shell from the wrong terminal emulator.
The `TERM` string required depends on the terminal emulator you use and thus
should only be set by it.

#### Building

Install all dependencies (listed above) and run `make`.

Ideally the code should compile fine if all dependencies are installed.

It has been tested on Cygwin (Windows), Debian and Arch Linux, all on x86-64.
Native Windows is currently **NOT** supported. I did some compilation tests
with the MinGW 64 compiler (MSYS2). However, even when compiling the code,
getting native 256 colors to work and getting all the unicode characters to
display properly wasn't something I was able to achieve during a long day.

#### Contributing

If you have any suggestions feel free to open up a pull request or just an
issue with some basic information. For issues I'm mostly focused on fixing bugs
and not really on any new features.

Please be reminded that this was a "C++ learning project" for me and therefore
the code is quite weird and probably contains a lot of "bad practices" in a few
places.
