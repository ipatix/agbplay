# agbplay
"agbplay" is a music player with Terminal interface for GBA ROMs that use the most common (aka mp2k/m4a) sound engine format.
The code itself is written in C++.

Quick overview:
![agbplay](https://cloud.githubusercontent.com/assets/8502545/24632845/faa2503c-18c5-11e7-84a3-cecec08e034a.png)

Controls:
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

Current state of things:
- ROMs can be loaded and scanned for the songtable automatically
- PCM playback works pretty much perfect, GB instruments sound great, but envelope curves are not 100% accurate
- Basic rendering to file done including dummy writing for benchmarking

Todo:
- Add missing key explanation for controls
- Fix wrong modulation curve (original uses triangle shaped waves, sine is being used currently)
- Change to an audio library that doesn't print ANYTHING messages on stdout
- Redo config system (perhaps in XML) to support games to share playlist (for games only differing in terms of language) and to allow multiple playlists for one game

Depenencies:
- boost
- portaudio
- (n)cursesw
- libsndfile

Compiling:
Install all the required dev packages and run the Makefile (make).

The code itself is written to be cross-platform compatible. That's why I've decided to go with Boost and portaudio.
I personally have tested it on 64 bit Cygwin (Windows) and 64 bit Debian Linux.
Native Windows support with Visual Studio is NOT supported by me and I NEVER will. Getting terminal things to work on Windows with UTF-8, colors and resizing terminal just doesn't work.
