cmake_minimum_required(VERSION 3.10)

project(agbplay VERSION 1.0.0)

option(ENABLE_ADDRESS_SANITIZER "Enable Address Sanitizer" OFF)

add_subdirectory("src/agbplay")
add_subdirectory("src/agbplay-util")
if(NOT WIN32)
    add_subdirectory("src/agbplay-nc")
endif()
add_subdirectory("src/agbplay-gui")
add_subdirectory("src/tests")
