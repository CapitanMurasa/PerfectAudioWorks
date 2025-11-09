cmake_minimum_required(VERSION 3.10)
project(PerfectAudioWorks LANGUAGES CXX C)

# --- Compiler Settings ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- Qt Setup ---
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# --- CMake Options for Decoders ---
option(ENABLE_SNDFILE "Enable libsndfile support" ON)
option(ENABLE_MPG123 "Enable mpg123 support" ON)

# --- Source Files ---
set(SOURCES
    src/main.cpp
    src/AudioPharser/PortAudioHandler.cpp
    src/AudioPharser/CodecHandler.c
    src/AudioPharser/portaudio_backend.c
    src/miscellaneous/file.cpp
)

# --- This block handles both Linux/WSL and MinGW ---
if(MINGW)
    message(STATUS "Using MinGW (pacman) configuration.")
    add_definitions(-D_WIN32) # <-- important for UTF-16 conversions
else()
    message(STATUS "Using Unix/Linux (WSL) configuration.")
endif()

# --- PortAudio ---
find_path(PORTAUDIO_INCLUDE_DIR portaudio.h)
find_library(PORTAUDIO_LIB NAMES portaudio)
if(PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIB)
    include_directories(${PORTAUDIO_INCLUDE_DIR})
    set(PORTAUDIO_TARGET ${PORTAUDIO_LIB})
else()
    message(FATAL_ERROR "PortAudio not found. Install 'portaudio19-dev' or 'mingw-w64-ucrt-x86_64-portaudio'")
endif()

# --- TagLib (Internal Build) ---
set(TAGLIB_DIR "${CMAKE_SOURCE_DIR}/extern/taglib")
if(EXISTS ${TAGLIB_DIR}/CMakeLists.txt)
    message(STATUS "Using bundled TagLib from ${TAGLIB_DIR}")

    # --- THIS IS THE FIX ---
    # Force TagLib's Windows Unicode support ON before including it.
    if(WIN32 OR MINGW)
        set(ENABLE_WIN32_ENCODING ON CACHE BOOL "Force Win32 encoding for TagLib")
    endif()
    # --- END OF FIX ---

    add_subdirectory(${TAGLIB_DIR} EXCLUDE_FROM_ALL)
    include_directories(${TAGLIB_DIR}/taglib)
    set(TAGLIB_TARGET tag)
else()
    message(FATAL_ERROR "TagLib source not found. Clone it under extern/taglib.")
endif()


# --- SndFile & MPG123 ---
if(ENABLE_SNDFILE)
    find_path(SNDFILE_INCLUDE_DIR sndfile.h)
    find_library(SNDFILE_LIB NAMES sndfile)
    if(SNDFILE_INCLUDE_DIR AND SNDFILE_LIB)
        message(STATUS "libsndfile enabled")
        add_definitions(-DENABLE_SNDFILE)
        include_directories(${SNDFILE_INCLUDE_DIR})
        set(SNDFILE_TARGET ${SNDFILE_LIB})
        list(APPEND SOURCES src/AudioPharser/libsndfiledecoder.c)
    endif()
endif()

if(ENABLE_MPG123)
    find_path(MPG123_INCLUDE_DIR mpg123.h)
    find_library(MPG123_LIB NAMES mpg123)
    if(MPG123_INCLUDE_DIR AND MPG123_LIB)
        message(STATUS "mpg123 enabled")
        add_definitions(-DENABLE_MPG123)
        include_directories(${MPG123_INCLUDE_DIR})
        set(MPG123_TARGET ${MPG123_LIB})
        list(APPEND SOURCES src/AudioPharser/mpg123decoder.c)
    endif()
endif()

# --- GUI Sources ---
list(APPEND SOURCES
    src/PAW_GUI/about_paw_gui.ui
    src/PAW_GUI/about_paw_gui.cpp
    src/PAW_GUI/settings_paw_gui.ui
    src/PAW_GUI/settings_paw_gui.cpp
    src/PAW_GUI/main_paw_widget.ui
    src/PAW_GUI/main_paw_widget.cpp
)

# --- Executable Definition ---
add_executable(PAW ${SOURCES})

# --- Linking ---
target_link_libraries(PAW PRIVATE
    Qt6::Core
    Qt6::Widgets
    ${PORTAUDIO_TARGET}
    ${TAGLIB_TARGET}
)

if(SNDFILE_TARGET)
    target_link_libraries(PAW PRIVATE ${SNDFILE_TARGET})
endif()

if(MPG123_TARGET)
    target_link_libraries(PAW PRIVATE ${MPG123_TARGET})
endif()

# --- Windows Specific ---
if (WIN32)
    target_link_libraries(PAW PRIVATE Ws2_32)
endif()
