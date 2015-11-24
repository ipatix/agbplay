# agbplay
"agbplay" is a music player for GBA ROMs that use the most common (aka mp2k/m4a) sound engine format.
The code itself is written in C++.

Current state of things:
- The GUI is more or less finished
- ROMs can be loaded and scanned for the songtable
- Work on the player itself isn't really started yet

Depenencies:
- Boost
- portaudio

Compiling:
Since the code isn't really at a functional state yet I might commit uncompileable code. Therefore, if you attempt to compile don't be surprised if you get a bunch of errors.
The code itself is written to be cross-platform compatible. That's why I've decided to go with Boost and portaudio.

I might eventually move from pure Makefiles to CMake, but for now there is other priorities.
