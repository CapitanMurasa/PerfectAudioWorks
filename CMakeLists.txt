cmake_minimum_required(VERSION 3.10)
project(PerfectAudioWorks VERSION 0.1.0 LANGUAGES CXX C)

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

# --- FFmpeg (Custom Minimal Build) ---
if(ENABLE_FFMPEG)
    message(STATUS "Configuring Custom FFmpeg...")
    
    get_filename_component(FFMPEG_ROOT "${CMAKE_SOURCE_DIR}/extern/ffmpeg_minimal" ABSOLUTE)
    
    
    add_library(FFmpeg::avcodec SHARED IMPORTED)
    set_target_properties(FFmpeg::avcodec PROPERTIES
        IMPORTED_IMPLIB "${FFMPEG_ROOT}/bin/avcodec.lib"        
        IMPORTED_LOCATION "${FFMPEG_ROOT}/bin/avcodec-62.dll"  
        INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_ROOT}/include"
    )
    
    add_library(FFmpeg::avformat SHARED IMPORTED)
    set_target_properties(FFmpeg::avformat PROPERTIES
        IMPORTED_IMPLIB "${FFMPEG_ROOT}/bin/avformat.lib"
        IMPORTED_LOCATION "${FFMPEG_ROOT}/bin/avformat-62.dll"  
        INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_ROOT}/include"
    )

    add_library(FFmpeg::avutil SHARED IMPORTED)
    set_target_properties(FFmpeg::avutil PROPERTIES
        IMPORTED_IMPLIB "${FFMPEG_ROOT}/bin/avutil.lib"
        IMPORTED_LOCATION "${FFMPEG_ROOT}/bin/avutil-60.dll"    
        INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_ROOT}/include"
    )
    
    add_library(FFmpeg::swresample SHARED IMPORTED)
    set_target_properties(FFmpeg::swresample PROPERTIES
        IMPORTED_IMPLIB "${FFMPEG_ROOT}/bin/swresample.lib"
        IMPORTED_LOCATION "${FFMPEG_ROOT}/bin/swresample-6.dll" 
        INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_ROOT}/include"
    )
    

    target_compile_definitions(PAW PRIVATE ENABLE_FFMPEG)
    target_sources(PAW PRIVATE src/AudioPharser/ffmpegdecoder.c)
    
    target_link_libraries(PAW PRIVATE 
        FFmpeg::avcodec 
        FFmpeg::avformat 
        FFmpeg::avutil 
        FFmpeg::swresample
    )
    
    if(WIN32)
        file(GLOB FFMPEG_DLLS "${FFMPEG_ROOT}/bin/*.dll")
        add_custom_command(TARGET PAW POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${FFMPEG_DLLS}
            $<TARGET_FILE_DIR:PAW>
            COMMENT "Deploying Custom FFmpeg DLLs..."
        )
    endif()
endif()

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

include(GNUInstallDirs)

install(TARGETS PAW RUNTIME DESTINATION bin)

if(WIN32)
    set(KNOWN_GOOD_BIN_DIR "${CMAKE_SOURCE_DIR}/out/install/Mingw64-Release/bin")

    install(DIRECTORY "${KNOWN_GOOD_BIN_DIR}/"
        DESTINATION bin
        FILES_MATCHING PATTERN "*.dll"
    )
    
    install(DIRECTORY "${KNOWN_GOOD_BIN_DIR}/platforms" DESTINATION bin)
    install(DIRECTORY "${KNOWN_GOOD_BIN_DIR}/styles"    DESTINATION bin)
    install(DIRECTORY "${KNOWN_GOOD_BIN_DIR}/imageformats" DESTINATION bin)

endif()

if(UNIX AND NOT APPLE)
    install(FILES "${CMAKE_SOURCE_DIR}/paw.desktop" DESTINATION share/applications)
    install(FILES "${CMAKE_SOURCE_DIR}/src/assets/paw.png" DESTINATION share/icons/hicolor/256x256/apps)
endif()

# --- NSIS Installer ---
set(CPACK_PACKAGE_NAME "PerfectAudioWorks")
set(CPACK_PACKAGE_VENDOR "CapitanMurasa")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Lightweight Audio Player")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "PerfectAudioWorks")
set(CPACK_PACKAGE_CONTACT "https://github.com/CapitanMurasa/PerfectAudioWorks")

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "PerfectAudioWorks")
    set(CPACK_NSIS_PACKAGE_NAME "PerfectAudioWorks ${PROJECT_VERSION}")
    set(CPACK_NSIS_LICENSE_PAGE_HEADER "PerfectAudioWorks License Agreement")
    
    set(CPACK_NSIS_CREATE_ICONS_EXTRA "
        CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\PerfectAudioWorks.lnk' '$INSTDIR\\\\bin\\\\PAW.exe'
    ")
    
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
        CreateShortCut '$DESKTOP\\\\PerfectAudioWorks.lnk' '$INSTDIR\\\\bin\\\\PAW.exe'
    ")
    
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
        Delete '$DESKTOP\\\\PerfectAudioWorks.lnk'
        Delete '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\PerfectAudioWorks.lnk'
    ")
    

endif()

if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "CapitanMurasa")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON) 
    set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE") 
endif()

include(CPack)