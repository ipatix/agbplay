# agbplay
"agbplay" is a music player with Terminal interface for GBA ROMs that use the most common (aka mp2k/m4a) sound engine format.
The code itself is written in C++.

Current state of things:
- ROMs can be loaded and scanned for the songtable automatically
- PCM playback works pretty much perfect, GB instruments sound great, but envelope curves are not 100% accurate
- Basic rendering to file done including dummy writing for benchmarking

Todo:
- Add missing key explanation for controls
- Implement Reverb for missing games
- Change to an audio library that doesn't have annoying package dependecy issues and doesn't print ANYTHING messages on stdout
- Redo config system (perhaps in XML) to support games to share playlist (for games only differing in terms of language) and to allow multiple playlists for one game

Depenencies:
- boost
- portaudio
- (n)curses
- libsndfile

Compiling:
Install all the required dev packages and run the Makefile.
However, because on all of my computers I had issues installing ''portaudio-dev', so in order to compile agbplay you'll need to do the following:

Clone the code repositories of agbplay and portaudio to a folder structure like this:

.
├── agbplay
└── portaudio

Go into the portaudio folder and execute "./configure && make" to build portaudio. 
Now you should be able to do "make" in the agbplay folder and hopefully everything compiles correctly

The code itself is written to be cross-platform compatible. That's why I've decided to go with Boost and portaudio.
However, for now the main development is done on Linux. So as long as there is not a Visual Studio project available the best chances are probably to use Cygwin for compiling on Windows.

I might eventually move from pure Makefiles to CMake, but for now there is other priorities.
