cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 23)

include_directories("${CMAKE_CURRENT_LIST_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
link_libraries(agbplay)

add_executable(test-resampler-sinc TestResamplerSinc.cpp)
target_compile_options(test-resampler-sinc PRIVATE -Wall -Wextra -Wconversion)
