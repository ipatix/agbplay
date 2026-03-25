cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 23)

find_package(argparse)
if(NOT argparse_FOUND)
    message(STATUS "argparse not found locally. Fetching from GitHub...")
    include(FetchContent)
    FetchContent_Declare(
        argparse
        GIT_REPOSITORY https://github.com/p-ranav/argparse.git
        GIT_TAG 3eda91b2e1ce7d569f84ba295507c4cd8fd96910 # v3.2
    )
    FetchContent_MakeAvailable(argparse)
else()
    message(STATUS "  Found argparse, version ${argparse_VERSION}")
endif()

file(GLOB_RECURSE AGBPLAY_CLI_SOURCES "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/../agbplay-common/*.cpp")
#file(GLOB_RECURSE AGBPLAY_CLI_HEADERS "${CMAKE_CURRENT_LIST_DIR}/*.h" "${CMAKE_CURRENT_LIST_DIR}/*.hpp")

add_executable(agbplay-cli ${AGBPLAY_CLI_SOURCES})

target_compile_options(agbplay-cli PRIVATE -Wall -Wextra -Wconversion)

if(ENABLE_ADDRESS_SANITIZER)
    target_compile_options(agbplay-cli PRIVATE -fsanitize=address)
    target_link_options(agbplay-cli PRIVATE -fsanitize=address)
endif()

target_link_libraries(agbplay-cli
    PRIVATE
    agbplay
    argparse::argparse
)
