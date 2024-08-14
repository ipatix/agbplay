cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)

include(FindPkgConfig)
find_package(PkgConfig REQUIRED)
pkg_check_modules(portaudio REQUIRED IMPORTED_TARGET portaudiocpp)

file(GLOB_RECURSE AGBPLAY_UTIL_SOURCES "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

add_library(agbplay-util STATIC ${AGBPLAY_UTIL_SOURCES})

target_compile_options(agbplay-util PRIVATE -Wall -Wextra -Wconversion)
target_include_directories(agbplay-util PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_link_libraries(agbplay-util
    PRIVATE
    agbplay
    PkgConfig::portaudio
)
