cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)

include(FindPkgConfig)
find_package(PkgConfig REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Boost CONFIG REQUIRED)
find_package(fmt REQUIRED)
find_package(libzip REQUIRED)
pkg_check_modules(sndfile REQUIRED IMPORTED_TARGET sndfile)
pkg_check_modules(zlib REQUIRED IMPORTED_TARGET zlib)

# configure heaer
find_package(Git REQUIRED)
execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --always --dirty
    COMMAND_ERROR_IS_FATAL ANY
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    OUTPUT_VARIABLE GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(TIMESTAMP BUILD_YEAR "%Y")
configure_file(Version.hpp.in Version.hpp @ONLY)

file(GLOB_RECURSE AGBPLAY_SOURCES "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
set(AGBPLAY_SOURCES_AVX2 ${AGBPLAY_SOURCES})
list(FILTER AGBPLAY_SOURCES_AVX2 INCLUDE REGEX ".*AVX2\\.cpp")

add_library(agbplay SHARED ${AGBPLAY_SOURCES})

target_compile_options(agbplay PRIVATE -Wall -Wextra -Wconversion)
set_source_files_properties(${AGBPLAY_SOURCES_AVX2} PROPERTIES COMPILE_FLAGS -mavx2)
#target_compile_options(agbplay PRIVATE -Wall -Wextra -Wconversion -fsanitize=address)
#target_link_options(agbplay PRIVATE -fsanitize=address)
target_include_directories(agbplay PUBLIC "${CMAKE_CURRENT_LIST_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(agbplay
    PUBLIC
    fmt::fmt
    nlohmann_json::nlohmann_json
    PRIVATE
    Boost::boost
    PkgConfig::sndfile
    libzip::zip
    PkgConfig::zlib
    ${AGBPLAY_ADDITIONAL_LIBS}
)
