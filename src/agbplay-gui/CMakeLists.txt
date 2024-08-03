cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

include(FindPkgConfig)
find_package(PkgConfig REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
pkg_check_modules(portaudio REQUIRED IMPORTED_TARGET portaudiocpp)

# TODO configure header

file(GLOB_RECURSE AGBPLAY_GUI_SOURCES "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/../agbplay-common/*.cpp")
file(GLOB_RECURSE AGBPLAY_GUI_HEADERS "${CMAKE_CURRENT_LIST_DIR}/*.h" "${CMAKE_CURRENT_LIST_DIR}/*.hpp")
file(GLOB_RECURSE AGBPLAY_GUI_UIS "${CMAKE_CURRENT_LIST_DIR}/*.ui")
file(GLOB_RECURSE AGBPLAY_GUI_QRC "${CMAKE_CURRENT_LIST_DIR}/*.qrc")

add_executable(agbplay-gui ${AGBPLAY_GUI_SOURCES} ${AGBPLAY_GUI_HEADERS} ${AGBPLAY_GUI_UIS} ${AGBPLAY_GUI_QRC})

target_compile_options(agbplay-gui PRIVATE -Wall -Wextra -Wconversion)
#target_compile_options(agbplay-gui PRIVATE -Wall -Wextra -Wconversion -fsanitize=address)
#target_link_options(agbplay-gui PRIVATE -fsanitize=address)
target_include_directories(agbplay-gui PRIVATE "${CMAKE_CURRENT_LIST_DIR}" "${CMAKE_CURRENT_LIST_DIR}/../agbplay-common")
target_link_libraries(agbplay-gui
    PRIVATE
    agbplay
    Qt6::Core
    Qt6::Widgets
    PkgConfig::portaudio
)
