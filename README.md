# agbplay
"agbplay" is a music player with Terminal interface for GBA ROMs that use the most common (aka mp2k/m4a) sound engine format.
The code itself is written in C++.

Current state of things:
- The GUI is more or less finished
- ROMs can be loaded and scanned for the songtable
- PCM playback works pretty much perfect, GB instruments still broken as hell
- Reverb algorithm of Camelot games still not implemented
- Regular reverb algorithm falsly affects GB instruments
- Minor controls for actual playback are still missing

Depenencies:
- Boost
- portaudio

Compiling:
Since the code isn't really at a functional state yet I might commit uncompileable code. Therefore, if you attempt to compile don't be surprised if you get a bunch of errors.
The code itself is written to be cross-platform compatible. That's why I've decided to go with Boost and portaudio.
However, for now the main development is done on Linux. So as long as there is not a Visual Studio project available the best chances are probably to use Cygwin for compiling on Windows.

I might eventually move from pure Makefiles to CMake, but for now there is other priorities.
