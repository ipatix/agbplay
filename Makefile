.PHONY: all

all:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
	cmake --build build -j$$(nproc)

debug:
	cmake -S . -B build_dbg -DCMAKE_BUILD_TYPE=Debug
	cmake --build build_dbg -j$$(nproc)

install:
	cmake --install build
