.PHONY: all

all:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
	cmake --build build -j$$(nproc)

install:
	cmake --install build
