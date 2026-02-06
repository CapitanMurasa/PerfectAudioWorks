cmake_minimum_required(VERSION 3.10)
project(PerfectAudioWorks VERSION 0.0.9 LANGUAGES CXX C)

# --- Compiler Settings ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- Qt Setup ---
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# --- Decoders Options ---
option(ENABLE_FFMPEG "Enable FFmpeg support (Recommended)" ON)

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
    src/miscellaneous/misc.c

    # GUI Files
    src/PAW_GUI/about_paw_gui.ui
    src/PAW_GUI/about_paw_gui.cpp
    src/PAW_GUI/aboutfile_paw_gui.ui
    src/PAW_GUI/aboutfile_paw_gui.cpp
    src/PAW_GUI/settings_paw_gui.ui
    src/PAW_GUI/settings_paw_gui.cpp
    src/PAW_GUI/main_paw_widget.ui
    src/PAW_GUI/main_paw_widget.cpp
    
    # Scrolling Label components
    src/PAW_GUI/ScrollingLabel.h
    src/PAW_GUI/ScrollingLabel.cpp
)

# --- Executable Definition ---
if(WIN32)
    add_executable(PAW WIN32 ${SOURCES})
else()
    add_executable(PAW ${SOURCES})
endif()

target_include_directories(PAW PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PAW_GUI
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AudioPharser
    ${CMAKE_CURRENT_SOURCE_DIR}/src/miscellaneous
)

# --- Dependencies Setup ---

# 1. PortAudio
find_path(PORTAUDIO_INCLUDE_DIR portaudio.h)
find_library(PORTAUDIO_LIB NAMES portaudio)
if(PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIB)
    target_include_directories(PAW PRIVATE ${PORTAUDIO_INCLUDE_DIR})
    set(PORTAUDIO_TARGET ${PORTAUDIO_LIB})
else()
    message(FATAL_ERROR "PortAudio not found.")
endif()

# 2. FFmpeg (The Master Decoder)
if(ENABLE_FFMPEG)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavcodec libavformat libavutil libswresample)
    
    target_compile_definitions(PAW PRIVATE ENABLE_FFMPEG)
    target_sources(PAW PRIVATE src/AudioPharser/ffmpegdecoder.c)
    target_link_libraries(PAW PRIVATE PkgConfig::FFMPEG)
    message(STATUS "FFmpeg Support: ENABLED")
endif()

# 3. TagLib (Bundled)
set(TAGLIB_DIR "${CMAKE_SOURCE_DIR}/extern/taglib")
if(EXISTS ${TAGLIB_DIR}/CMakeLists.txt)
    add_subdirectory(${TAGLIB_DIR} EXCLUDE_FROM_ALL)
    target_include_directories(PAW PRIVATE ${TAGLIB_DIR}/taglib)
    set(TAGLIB_TARGET tag)
    if(WIN32 OR MINGW)
        target_compile_definitions(tag PRIVATE TAGLIB_WITH_WINDOWS_ENCODING)
    endif()
else()
    message(FATAL_ERROR "TagLib source not found.")
endif()

# 4. JSON (Bundled)
set(JSON_DIR "${CMAKE_SOURCE_DIR}/extern/json")
if(EXISTS ${JSON_DIR}/CMakeLists.txt)
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    add_subdirectory(${JSON_DIR} EXCLUDE_FROM_ALL)
else()
    message(FATAL_ERROR "nlohmann_json source not found.")
endif()

# 5. Optional Legacy Decoders
if(ENABLE_SNDFILE)
    find_path(SNDFILE_INCLUDE_DIR sndfile.h)
    find_library(SNDFILE_LIB NAMES sndfile)
    if(SNDFILE_INCLUDE_DIR AND SNDFILE_LIB)
        target_compile_definitions(PAW PRIVATE ENABLE_SNDFILE)
        target_include_directories(PAW PRIVATE ${SNDFILE_INCLUDE_DIR})
        target_sources(PAW PRIVATE src/AudioPharser/libsndfiledecoder.c)
        target_link_libraries(PAW PRIVATE ${SNDFILE_LIB})
    endif()
endif()

if(ENABLE_MPG123)
    find_path(MPG123_INCLUDE_DIR mpg123.h)
    find_library(MPG123_LIB NAMES mpg123)
    if(MPG123_INCLUDE_DIR AND MPG123_LIB)
        target_compile_definitions(PAW PRIVATE ENABLE_MPG123)
        target_include_directories(PAW PRIVATE ${MPG123_INCLUDE_DIR})
        target_sources(PAW PRIVATE src/AudioPharser/mpg123decoder.c)
        target_link_libraries(PAW PRIVATE ${MPG123_LIB})
    endif()
endif()

# --- Linking ---
target_link_libraries(PAW PRIVATE
    Qt6::Core
    Qt6::Widgets
    ${PORTAUDIO_TARGET}
    ${TAGLIB_TARGET}
    nlohmann_json::nlohmann_json  
)

if (WIN32)
    target_link_libraries(PAW PRIVATE Ws2_32)
    if(MINGW)
        add_definitions(-D_WIN32) 
    endif()
endif()

# --- Install & Package ---
include(GNUInstallDirs)
install(TARGETS PAW RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(WIN32)
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/deploy/bin/"
            DESTINATION bin
            FILES_MATCHING PATTERN "*.dll")
    
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/deploy/bin/platforms" DESTINATION bin)
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/deploy/bin/styles" DESTINATION bin)
endif()

# --- NSIS Installer ---
set(CPACK_GENERATOR "NSIS")
set(CPACK_PACKAGE_NAME "PerfectAudioWorks")
set(CPACK_PACKAGE_VENDOR "CapitanMurasa")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Audio Player")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "PerfectAudioWorks")
set(CPACK_NSIS_DISPLAY_NAME "PerfectAudioWorks")
set(CPACK_NSIS_PACKAGE_NAME "PerfectAudioWorks")
set(CPACK_NSIS_HELP_LINK "https://github.com/CapitanMurasa/PerfectAudioWorks")
set(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\PerfectAudioWorks.lnk' '$INSTDIR\\\\bin\\\\PAW.exe'")
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "CreateShortCut '$DESKTOP\\\\PerfectAudioWorks.lnk' '$INSTDIR\\\\bin\\\\PAW.exe'")
set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "Delete '$DESKTOP\\\\PerfectAudioWorks.lnk'")
set(CPACK_NSIS_WELCOME_TITLE "Welcome to PerfectAudioWorks installation wizard")

include(CPack)