cmake_minimum_required(VERSION 3.10)
project(PerfectAudioWorks LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- Global Static Flags ---
set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})
set(BUILD_SHARED_LIBS OFF)

# --- Qt Setup ---
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
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
)

# --- Platform Specifics ---
if(MINGW)
    add_definitions(-D_WIN32) 
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
    message(STATUS "PortAudio Static: ${PORTAUDIO_LIB}")
else()
    message(FATAL_ERROR "PortAudio static lib not found.")
endif()

# --- TagLib (Internal Build) ---
set(TAGLIB_DIR "${CMAKE_SOURCE_DIR}/extern/taglib")
if(EXISTS ${TAGLIB_DIR}/CMakeLists.txt)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    add_subdirectory(${TAGLIB_DIR} EXCLUDE_FROM_ALL)
    set(TAGLIB_TARGET tag)
else()
    message(FATAL_ERROR "TagLib source not found.")
endif()

# --- JSON (Header Only / Static) ---
set(JSON_DIR "${CMAKE_SOURCE_DIR}/extern/json")
if(EXISTS ${JSON_DIR}/CMakeLists.txt)
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    add_subdirectory(${JSON_DIR} EXCLUDE_FROM_ALL)
else()
    message(FATAL_ERROR "nlohmann_json source not found.")
endif()

# --- SndFile & MPG123 (Static) ---
if(ENABLE_SNDFILE)
    find_library(SNDFILE_LIB NAMES liblibsndfile.a libsndfile.a libsndfile)
    if(SNDFILE_LIB)
        add_definitions(-DENABLE_SNDFILE)
        add_definitions(-DSNDFILE_STATIC) 
        list(APPEND SOURCES src/AudioPharser/libsndfiledecoder.c)
    endif()
endif()

if(ENABLE_MPG123)
    find_library(MPG123_LIB NAMES libmpg123.a mpg123-static mpg123)
    if(MPG123_LIB)
        add_definitions(-DENABLE_MPG123)
        # Prevents the compiler from looking for a DLL
        add_definitions(-DMPG123_STATIC)
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

# --- Executable ---
if(WIN32)
    add_executable(PAW WIN32 ${SOURCES})
else()
    add_executable(PAW ${SOURCES})
endif()

# --- Linking ---
target_link_libraries(PAW PRIVATE
    Qt6::Core
    Qt6::Widgets
    PortAudio_Static
    ${TAGLIB_TARGET}
    nlohmann_json::nlohmann_json
    ${SNDFILE_LIB}
    ${MPG123_LIB}
)

# --- OS-Specific System Dependencies ---
if(WIN32)
    target_link_libraries(PAW PRIVATE Ws2_32 winmm ole32 advapi32 setupapi)
elseif(APPLE)
    target_link_libraries(PAW PRIVATE "-framework CoreAudio -framework AudioToolbox -framework AudioUnit -framework CoreFoundation -framework Carbon")
else()
    target_link_libraries(PAW PRIVATE pthread m dl)
endif()