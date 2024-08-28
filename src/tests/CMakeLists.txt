cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 20)

find_package(fmt REQUIRED)

include_directories("${CMAKE_CURRENT_LIST_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
link_libraries(agbplay fmt::fmt)

add_executable(test-resampler-sinc TestResamplerSinc.cpp)
target_compile_options(test-resampler-sinc PRIVATE -Wall -Wextra -Wconversion)
