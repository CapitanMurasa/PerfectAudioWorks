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

# --- Define empty targets (will be set in if/else) ---
set(PORTAUDIO_TARGET "")
set(SNDFILE_TARGET "")
set(MPG123_TARGET "")
set(TAGLIB_TARGET "")


# --- PortAudio and Dependency Logic ---

if(MSVC)
    message(STATUS "Using Windows/MSVC (find_path/find_library) configuration.")
    add_definitions(-D_WIN32) 


    set(VCPKG_ROOT "C:/Users/MOKOU/Documents/dev/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(VCPKG_WIN_TRIPLET "x64-windows")
    set(VCPKG_SEARCH_PATH "${VCPKG_ROOT}/packages/portaudio_${VCPKG_WIN_TRIPLET}")
    set(TAGLIB_SEARCH_PATH "${VCPKG_ROOT}/packages/taglib_${VCPKG_WIN_TRIPLET}")

    # PortAudio (Manual)
    find_path(PORTAUDIO_INCLUDE_DIR portaudio.h
        PATHS "${VCPKG_SEARCH_PATH}/include"
    )
    find_library(PORTAUDIO_LIB NAMES portaudio
        PATHS "${VCPKG_SEARCH_PATH}/lib"
    )
    if(PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIB)
        include_directories(${PORTAUDIO_INCLUDE_DIR})
        set(PORTAUDIO_TARGET ${PORTAUDIO_LIB})
    else()
        message(FATAL_ERROR "PortAudio (vcpkg) not found. Check VCPKG_ROOT path in CMakeLists.txt.")
    endif()

    # TagLib (Manual)
    find_path(TAGLIB_INCLUDE_DIR taglib/tag.h
        PATHS "${TAGLIB_SEARCH_PATH}/include"
    )
    find_library(TAGLIB_LIB NAMES tag
        PATHS "${TAGLIB_SEARCH_PATH}/lib"
    )
    if(TAGLIB_INCLUDE_DIR AND TAGLIB_LIB)
        include_directories(${TAGLIB_INCLUDE_DIR})
        set(TAGLIB_TARGET ${TAGLIB_LIB})
    else()
        message(FATAL_ERROR "TagLib (vcpkg) not found. Check VCPKG_ROOT path in CMakeLists.txt.")
    endif()

    # SndFile & MPG123 (vcpkg still handles these well)
    if(ENABLE_SNDFILE)
        find_package(SndFile CONFIG)
        if(SndFile_FOUND)
            add_definitions(-DENABLE_SNDFILE)
            set(SNDFILE_TARGET SndFile::sndfile)
            list(APPEND SOURCES src/AudioPharser/libsndfiledecoder.c)
        endif()
    endif()
    if(ENABLE_MPG123)
        find_package(mpg123 CONFIG)
        if(MPG123_FOUND)
            add_definitions(-DENABLE_MPG123)
            set(MPG123_TARGET MPG123::libmpg123)
            list(APPEND SOURCES src/AudioPharser/mpg123decoder.c)
        endif()
    endif()

else()
    # --- Unix/Linux (WSL) Configuration ---
    message(STATUS "Using Unix/Linux (find_path/find_library) configuration.")

    # PortAudio (Manual)
    find_path(PORTAUDIO_INCLUDE_DIR portaudio.h)
    find_library(PORTAUDIO_LIB NAMES portaudio)
    if(PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIB)
        include_directories(${PORTAUDIO_INCLUDE_DIR})
        set(PORTAUDIO_TARGET ${PORTAUDIO_LIB})
    else()
        message(FATAL_ERROR "PortAudio not found. Have you installed 'portaudio19-dev'?")
    endif()

    # TagLib (Manual)
    find_path(TAGLIB_INCLUDE_DIR taglib/tag.h)
    find_library(TAGLIB_LIB NAMES tag)
    if(TAGLIB_INCLUDE_DIR AND TAGLIB_LIB)
        include_directories(${TAGLIB_INCLUDE_DIR})
        set(TAGLIB_TARGET ${TAGLIB_LIB})
    else()
        message(FATAL_ERROR "TagLib not found. Have you installed 'libtag1-dev'?")
    endif()

    # SndFile & MPG123 (Manual)
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
endif()

# --- GUI Sources (Always Added) ---
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

# --- Universal Linking ---
target_link_libraries(PAW PRIVATE
    Qt6::Core
    Qt6::Widgets
    ${PORTAUDIO_TARGET}
    ${TAGLIB_TARGET}
)

# --- Conditional Linking ---
if(SNDFILE_TARGET)
    target_link_libraries(PAW PRIVATE ${SNDFILE_TARGET})
endif()
if(MPG123_TARGET)
    target_link_libraries(PAW PRIVATE ${MPG123_TARGET})
endif()

# --- Windows-Specific Linking ---
if (WIN32)
    target_link_libraries(PAW PRIVATE Ws2_32)
endif()