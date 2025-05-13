.PHONY: all clean format

all: release

clean:
	rm -rf build build_dbg

release:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build build -j$$(nproc)

debug:
	cmake -S . -B build_dbg -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build build_dbg -j$$(nproc)

install:
	cmake --install build

format:
	clang-format -i src/*/*.hpp src/*/*.cpp
