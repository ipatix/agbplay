# agbplay
"agbplay" is a music player with Terminal interface for GBA ROMs that use the most common (aka mp2k/m4a) sound engine format.
The code itself is written in C++.

Current state of things:
- The GUI is more or less finished
- ROMs can be loaded and scanned for the songtable
- PCM playback works pretty much perfect, GB instruments still broken as hell
- Reverb algorithm of Camelot games still not implemented
- regular reverb algorith seems to cause pop sounds here and there
- Minor controls for actual playback are still missing

Depenencies:
- Boost
- portaudio
- (n)curses

Compiling:
Install all the required dev packages and run the Makefile.
The code itself is written to be cross-platform compatible. That's why I've decided to go with Boost and portaudio.
However, for now the main development is done on Linux. So as long as there is not a Visual Studio project available the best chances are probably to use Cygwin for compiling on Windows.

I might eventually move from pure Makefiles to CMake, but for now there is other priorities.
