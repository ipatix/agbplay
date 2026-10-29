cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 23)

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
    COMMAND ${GIT_EXECUTABLE} describe --always --tags --dirty
    COMMAND_ERROR_IS_FATAL ANY
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    OUTPUT_VARIABLE GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(TIMESTAMP BUILD_YEAR "%Y")
configure_file(Version.hpp.in Version.hpp @ONLY)

set(AGBPLAY_DEFINES "")
file(GLOB_RECURSE AGBPLAY_SOURCES "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
set(AGBPLAY_SOURCES_AVX2 ${AGBPLAY_SOURCES})
list(FILTER AGBPLAY_SOURCES_AVX2 INCLUDE REGEX ".*AVX2\\.cpp")


# AVX2 code is only supported on x86)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64|i[3-6]86|x86)$")
    list(APPEND AGBPLAY_DEFINES "AGBPLAY_WITH_AVX2")
else()
    list(REMOVE_ITEM AGBPLAY_SOURCES ${AGBPLAY_SOURCES_AVX2})
    set(AGBPLAY_SOURCES_AVX2 "")
endif()

add_library(agbplay SHARED ${AGBPLAY_SOURCES})

target_compile_options(agbplay PRIVATE -Wall -Wextra -Wconversion)
target_compile_definitions(agbplay PUBLIC ${AGBPLAY_DEFINES})
set_source_files_properties(${AGBPLAY_SOURCES_AVX2} PROPERTIES COMPILE_FLAGS -mavx2)

if(ENABLE_ADDRESS_SANITIZER)
    target_compile_options(agbplay PRIVATE -fsanitize=address)
    target_link_options(agbplay PRIVATE -fsanitize=address)
endif()

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
