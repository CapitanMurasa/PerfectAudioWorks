cmake_minimum_required(VERSION 3.10)
project(PerfectAudioWorks LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

option(ENABLE_SNDFILE "Enable libsndfile support" ON)
option(ENABLE_MPG123 "Enable mpg123 support" ON)

set(SOURCES
    src/main.cpp
    src/AudioPharser/PortAudioHandler.cpp
    src/AudioPharser/CodecHandler.c
    src/AudioPharser/portaudio_backend.c
    src/miscellaneous/file.c
)

if(ENABLE_SNDFILE)
    list(APPEND SOURCES src/AudioPharser/libsndfiledecoder.c)
endif()

if(ENABLE_MPG123)
    list(APPEND SOURCES src/AudioPharser/mpg123decoder.c)
endif()

list(APPEND SOURCES
    src/PAW_GUI/about_paw_gui.ui
    src/PAW_GUI/about_paw_gui.cpp
    src/PAW_GUI/settings_paw_gui.ui
    src/PAW_GUI/settings_paw_gui.cpp
    src/PAW_GUI/main_paw_widget.ui
    src/PAW_GUI/main_paw_widget.cpp
)


if(MSVC)
    message(STATUS "Using Windows/MSVC configuration.")
    add_definitions(-D_WIN32) 

    find_package(portaudio CONFIG REQUIRED)
    
    if(TARGET portaudio::portaudio)
        set(PORTAUDIO_TARGET portaudio::portaudio)
    elseif(TARGET portaudio)
        set(PORTAUDIO_TARGET portaudio)
    else()
        message(FATAL_ERROR "PortAudio config found, but neither target 'portaudio::portaudio' nor 'portaudio' exists.")
    endif()
    
    if(PORTAUDIO_INCLUDE_DIRS)
        include_directories(${PORTAUDIO_INCLUDE_DIRS})
    endif()
    
    if(ENABLE_SNDFILE)
        find_package(SndFile CONFIG)
        if(SndFile_FOUND)
            add_definitions(-DENABLE_SNDFILE)
            set(SNDFILE_TARGET SndFile::sndfile)
        endif()
    endif()
    if(ENABLE_MPG123)
        find_package(mpg123 CONFIG)
        if(MPG123_FOUND)
            add_definitions(-DENABLE_MPG123)
            set(MPG123_TARGET MPG123::libmpg123)
        endif()
    endif()

else()
    message(STATUS "Using Unix/Linux (find_path/find_library) configuration.")

    find_path(PORTAUDIO_INCLUDE_DIR portaudio.h)
    find_library(PORTAUDIO_LIB NAMES portaudio)
    if(PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIB)
        include_directories(${PORTAUDIO_INCLUDE_DIR})
        set(PORTAUDIO_TARGET ${PORTAUDIO_LIB}) 
    else()
        message(FATAL_ERROR "PortAudio not found. Have you installed 'portaudio19-dev'?")
    endif()


    if(ENABLE_SNDFILE)
        find_path(SNDFILE_INCLUDE_DIR sndfile.h)
        find_library(SNDFILE_LIB NAMES sndfile)
        if(SNDFILE_INCLUDE_DIR AND SNDFILE_LIB)
            message(STATUS "libsndfile enabled")
            add_definitions(-DENABLE_SNDFILE)
            include_directories(${SNDFILE_INCLUDE_DIR})
            set(SNDFILE_TARGET ${SNDFILE_LIB})
        else()
            message(WARNING "libsndfile requested but not found, disabling")
            set(ENABLE_SNDFILE OFF)
        endif()
    endif()

    # MPG123
    if(ENABLE_MPG123)
        find_path(MPG123_INCLUDE_DIR mpg123.h)
        find_library(MPG123_LIB NAMES mpg123)
        if(MPG123_INCLUDE_DIR AND MPG123_LIB)
            message(STATUS "mpg123 enabled")
            add_definitions(-DENABLE_MPG123)
            include_directories(${MPG123_INCLUDE_DIR})
            set(MPG123_TARGET ${MPG123_LIB}) 
        else()
            message(WARNING "mpg123 requested but not found, disabling")
            set(ENABLE_MPG123 OFF)
        endif()
    endif()
endif()



add_executable(PAW ${SOURCES})


target_link_libraries(PAW PRIVATE
    Qt6::Core
    Qt6::Widgets
    ${PORTAUDIO_TARGET}
    ${SNDFILE_TARGET}
    ${MPG123_TARGET}
)


if (WIN32)
    target_link_libraries(PAW PRIVATE Ws2_32)
endif()