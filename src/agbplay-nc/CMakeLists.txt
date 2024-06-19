cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)

include(FindPkgConfig)
find_package(PkgConfig REQUIRED)
find_package(jsoncpp REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Boost REQUIRED)
find_package(fmt REQUIRED)
pkg_check_modules(ncurses REQUIRED IMPORTED_TARGET ncursesw)
pkg_check_modules(sndfile REQUIRED IMPORTED_TARGET sndfile)
pkg_check_modules(portaudio REQUIRED IMPORTED_TARGET portaudiocpp)

# TODO configure header

file(GLOB_RECURSE AGBPLAY_NC_SOURCES "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
#file(GLOB_RECURSE AGBPLAY_NC_HEADERS "${CMAKE_CURRENT_LIST_DIR}/*.h" "${CMAKE_CURRENT_LIST_DIR}/*.hpp")

add_executable(agbplay-nc ${AGBPLAY_NC_SOURCES})

target_compile_options(agbplay-nc PRIVATE -Wall -Wextra -Wconversion)
target_include_directories(agbplay-nc PRIVATE "${CMAKE_CURRENT_LIST_DIR}")
target_link_libraries(agbplay-nc
    JsonCpp::JsonCpp
    Boost::boost
    fmt::fmt
    nlohmann_json::nlohmann_json
    PkgConfig::sndfile
    PkgConfig::portaudio
    PkgConfig::ncurses
)
