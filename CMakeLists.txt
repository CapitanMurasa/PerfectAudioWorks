cmake_minimum_required(VERSION 3.10)
project(PerfectAudioWorks LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- Global Static Flags ---
# Force find_library to prefer .a over .dll.a/dll
set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_STATIC_LIBRARY_SUFFIX})
set(BUILD_SHARED_LIBS OFF)

# --- Qt Setup ---
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# --- CMake Options ---
option(ENABLE_SNDFILE "Enable libsndfile support" ON)
option(ENABLE_MPG123 "Enable mpg123 support" ON)

# --- Source Files ---
set(SOURCES
    src/main.cpp
    src/AudioPharser/PortAudioHandler.cpp
    src/AudioPharser/CodecHandler.c
    src/AudioPharser/portaudio_backend.c
    src/miscellaneous/file.cpp 
    src/miscellaneous/json.cpp
    src/PAW_GUI/about_paw_gui.cpp
    src/PAW_GUI/settings_paw_gui.cpp
    src/PAW_GUI/main_paw_widget.cpp
)

# --- Platform Specifics ---
if(MINGW)
    add_definitions(-D_WIN32) 
    # Ensure the final executable is strictly static
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
endif()

# --- PortAudio (Static) ---
find_path(PORTAUDIO_INCLUDE_DIR portaudio.h)
find_library(PORTAUDIO_LIB NAMES libportaudio.a portaudio_static portaudio)

if(PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIB)
    add_library(PortAudio_Static STATIC IMPORTED)
    set_target_properties(PortAudio_Static PROPERTIES 
        IMPORTED_LOCATION "${PORTAUDIO_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${PORTAUDIO_INCLUDE_DIR}")
else()
    message(FATAL_ERROR "PortAudio static lib not found.")
endif()

# --- SndFile & MPG123 (Static) ---
set(EXTRA_LIBS "")

if(ENABLE_SNDFILE)
    find_library(SNDFILE_LIB NAMES liblibsndfile.a libsndfile.a libsndfile)
    if(SNDFILE_LIB)
        add_definitions(-DENABLE_SNDFILE -DSNDFILE_STATIC)
        list(APPEND SOURCES src/AudioPharser/libsndfiledecoder.c)
        list(APPEND EXTRA_LIBS ${SNDFILE_LIB})
        
        # SndFile Static Dependencies (Required for linking)
        find_library(FLAC_LIB NAMES FLAC libFLAC)
        find_library(VORBIS_LIB NAMES vorbis libvorbis)
        find_library(VORBISENC_LIB NAMES vorbisenc libvorbisenc)
        find_library(OGG_LIB NAMES ogg libogg)
        find_library(OPUS_LIB NAMES opus libopus)
        find_library(LAME_LIB NAMES mp3lame libmp3lame)
        
        list(APPEND EXTRA_LIBS ${FLAC_LIB} ${VORBISENC_LIB} ${VORBIS_LIB} ${OGG_LIB} ${OPUS_LIB} ${LAME_LIB})
    endif()
endif()

if(ENABLE_MPG123)
    find_library(MPG123_LIB NAMES libmpg123.a mpg123-static mpg123)
    if(MPG123_LIB)
        add_definitions(-DENABLE_MPG123 -DMPG123_STATIC)
        list(APPEND SOURCES src/AudioPharser/mpg123decoder.c)
        list(APPEND EXTRA_LIBS ${MPG123_LIB})
    endif()
endif()

# --- Internal Dependencies ---
add_subdirectory(extern/taglib EXCLUDE_FROM_ALL)
add_subdirectory(extern/json EXCLUDE_FROM_ALL)

# --- Executable ---
if(WIN32)
    # Adding WIN32 ensures it opens as a GUI app (no console)
    add_executable(PAW WIN32 ${SOURCES})
else()
    add_executable(PAW ${SOURCES})
endif()

# --- Linking ---
target_link_libraries(PAW PRIVATE
    Qt6::Widgets
    Qt6::Gui
    Qt6::Core
    Qt6::EntryPointImplementation
    PortAudio_Static
    tag
    nlohmann_json::nlohmann_json
    ${EXTRA_LIBS}
)

# --- OS-Specific System Dependencies ---
if(WIN32)
    # shlwapi is required by mpg123; others are for PortAudio/Qt
    target_link_libraries(PAW PRIVATE Ws2_32 winmm ole32 advapi32 setupapi shlwapi userenv mpr d3d11 dxgi dxguid d3d12 z)
elseif(APPLE)
    target_link_libraries(PAW PRIVATE "-framework CoreAudio -framework AudioToolbox -framework AudioUnit -framework CoreFoundation -framework Carbon")
else()
    target_link_libraries(PAW PRIVATE pthread m dl)
endif()