cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)

# TODO configure header

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
)
