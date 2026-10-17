cmake_minimum_required(VERSION 3.10)
project(PerfectAudioWorks VERSION 0.1.4 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- Qt Dependency ---
if (UNIX)
    find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core Network Sql DBus OpenGLWidgets)
else()
    find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Sql OpenGLWidgets)
endif()

set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(Python3_USE_STATIC_LIBS OFF)
find_package(Python3 COMPONENTS Interpreter Development.Embed REQUIRED)

option(ENABLE_FFMPEG "Enable FFmpeg support (Recommended)" ON)
option(ENABLE_SNDFILE "Enable libsndfile support" ON)
option(ENABLE_MPG123 "Enable mpg123 support" ON)

set(SOURCES
    src/main.cpp
    src/assets/resources.rc
    src/assets.qrc
    src/AudioPharser/AudioRingBuffer.c
    src/AudioPharser/PortAudioHandler.cpp
    src/AudioPharser/CodecHandler.c
    src/AudioPharser/portaudio_backend.c
    src/miscellaneous/file.cpp
    src/miscellaneous/json.cpp
    src/miscellaneous/misc.c
    src/miscellaneous/DatabaseManager.cpp
    src/PAW_GUI/about_paw_gui.ui
    src/PAW_GUI/about_paw_gui.cpp
    src/PAW_GUI/aboutfile_paw_gui.ui
    src/PAW_GUI/aboutfile_paw_gui.cpp
    src/PAW_GUI/settings_paw_gui.ui
    src/PAW_GUI/settings_paw_gui.cpp
    src/PAW_GUI/main_paw_widget.ui
    src/PAW_GUI/main_paw_widget.cpp
    src/PAW_GUI/playlist_paw_manager.ui
    src/PAW_GUI/playlist_paw_manager.cpp
    src/PAW_GUI/loadingplaylists.cpp
    src/PAW_GUI/loadingplaylists.ui
    src/PAW_GUI/ScrollingLabel.h
    src/PAW_GUI/ScrollingLabel.cpp
    src/PAW_GUI/visualizer_widget.cpp
    src/miscellaneous/python_bindings.cpp
    src/miscellaneous/PythonEvent.cpp
    src/PAW_GUI/GlobalLinuxKeys.h
)

if(WIN32)
    add_executable(PAW WIN32 ${SOURCES})
else()
    add_executable(PAW ${SOURCES})
endif()

if(UNIX)
    set_target_properties(PAW PROPERTIES
        ENABLE_EXPORTS ON
        INSTALL_RPATH "/usr/local/lib"
        BUILD_WITH_INSTALL_RPATH FALSE
        BUILD_RPATH "${FFMPEG_ROOT}/lib"
    )
endif()

target_include_directories(PAW PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PAW_GUI
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AudioPharser
    ${CMAKE_CURRENT_SOURCE_DIR}/src/miscellaneous
)

find_path(PORTAUDIO_INCLUDE_DIR portaudio.h)
find_library(PORTAUDIO_LIB NAMES portaudio)
if(PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIB)
    target_include_directories(PAW PRIVATE ${PORTAUDIO_INCLUDE_DIR})
    set(PORTAUDIO_TARGET ${PORTAUDIO_LIB})
else()
    message(FATAL_ERROR "PortAudio not found.")
endif()

if(ENABLE_FFMPEG)
    if(WIN32)
        get_filename_component(FFMPEG_ROOT "${CMAKE_SOURCE_DIR}/extern/ffmpeg_minimal" ABSOLUTE)
        set(LIB_SUFFIX ".lib")
        set(BIN_SUFFIX ".dll")
        set(LIB_PREFIX "")
    elseif(UNIX)
        get_filename_component(FFMPEG_ROOT "${CMAKE_SOURCE_DIR}/extern/ffmpeg_minimal_linux" ABSOLUTE)
        set(LIB_SUFFIX ".so")
        set(BIN_SUFFIX ".so")
        set(LIB_PREFIX "lib")
    endif()

    macro(setup_ffmpeg_target name version)
        add_library(FFmpeg::${name} SHARED IMPORTED)
        if(WIN32)
            set_target_properties(FFmpeg::${name} PROPERTIES
                IMPORTED_IMPLIB "${FFMPEG_ROOT}/bin/${name}${LIB_SUFFIX}"
                IMPORTED_LOCATION "${FFMPEG_ROOT}/bin/${name}-${version}${BIN_SUFFIX}"
                INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_ROOT}/include"
            )
        else()
            set_target_properties(FFmpeg::${name} PROPERTIES
                IMPORTED_LOCATION "${FFMPEG_ROOT}/lib/${LIB_PREFIX}${name}${LIB_SUFFIX}"
                INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_ROOT}/include"
            )
        endif()
    endmacro()

    setup_ffmpeg_target(avcodec    62)
    setup_ffmpeg_target(avformat   62)
    setup_ffmpeg_target(avutil     60)
    setup_ffmpeg_target(swresample 6)

    target_compile_definitions(PAW PRIVATE ENABLE_FFMPEG)
    target_sources(PAW PRIVATE src/AudioPharser/ffmpegdecoder.c)
    target_link_libraries(PAW PRIVATE FFmpeg::avcodec FFmpeg::avformat FFmpeg::avutil FFmpeg::swresample)

    if(UNIX)
        install(DIRECTORY "${FFMPEG_ROOT}/lib/" DESTINATION lib FILES_MATCHING PATTERN "*.so*")
    endif()
endif()

# --- TagLib ---
find_path(TAGLIB_INCLUDE_DIR taglib/taglib.h)
find_library(TAGLIB_LIB NAMES tag)
if(TAGLIB_INCLUDE_DIR AND TAGLIB_LIB)
    target_include_directories(PAW PRIVATE ${TAGLIB_INCLUDE_DIR})
    set(TAGLIB_TARGET ${TAGLIB_LIB})
else()
    message(FATAL_ERROR "TagLib system package not found.")
endif()

# --- nlohmann_json ---
set(JSON_DIR "${CMAKE_SOURCE_DIR}/extern/json")
if(EXISTS ${JSON_DIR}/CMakeLists.txt)
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    add_subdirectory(${JSON_DIR} EXCLUDE_FROM_ALL)
else()
    message(FATAL_ERROR "nlohmann_json source not found.")
endif()

# --- Pybind11 ---
set(PYBIND11_DIR "${CMAKE_SOURCE_DIR}/extern/pybind11")
if(EXISTS ${PYBIND11_DIR}/CMakeLists.txt)
    add_subdirectory(${PYBIND11_DIR})
    target_link_libraries(PAW PRIVATE pybind11::embed)
else()
    message(WARNING "Pybind11 submodule not found.")
endif()

# --- KissFFT ---
set(KISSFFT_DIR "${CMAKE_SOURCE_DIR}/extern/kissfft")
if(EXISTS ${KISSFFT_DIR}/CMakeLists.txt)
    # Force static build, disable tools/tests for faster compile times
    set(KISSFFT_STATIC ON CACHE BOOL "Build KissFFT as static library" FORCE)
    set(KISSFFT_TEST OFF CACHE BOOL "Disable KissFFT tests" FORCE)
    set(KISSFFT_TOOLS OFF CACHE BOOL "Disable KissFFT tools" FORCE)
    set(KISSFFT_DATATYPE "float" CACHE STRING "Use float data type" FORCE)
    
    add_subdirectory(${KISSFFT_DIR} EXCLUDE_FROM_ALL)
    
    # Include both root (for kiss_fft.h) and tools (for kiss_fftr.h)
    target_include_directories(PAW PRIVATE 
        ${KISSFFT_DIR}
        ${KISSFFT_DIR}/tools
    )
else()
    message(FATAL_ERROR "KissFFT source not found. Did you forget to run 'git submodule update --init'?")
endif()

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

set(COMMON_LIBS
    pybind11::embed
    Python3::Python
    ${PORTAUDIO_TARGET}
    ${TAGLIB_TARGET}
    nlohmann_json::nlohmann_json
    kissfft  # <-- Added KissFFT to the linker
)

target_link_libraries(PAW PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::OpenGLWidgets
    ${COMMON_LIBS}
)

if(UNIX)
    target_link_libraries(PAW PRIVATE Qt6::DBus)
endif()

if (WIN32)
    target_link_libraries(PAW PRIVATE Ws2_32)
    if(MINGW)
        add_definitions(-D_WIN32)
    endif()
endif()

include(GNUInstallDirs)
install(TARGETS PAW RUNTIME DESTINATION bin)

if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB;RPM")
    
    set(CPACK_PACKAGE_CONTACT "CapitanMurasa <vladislavkolodiy45@gmail.com>")
    set(CPACK_PACKAGE_HOMEPAGE "https://github.com/CapitanMurasa/PerfectAudioWorks")

    # --- DEB ---
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${CPACK_PACKAGE_CONTACT})
    set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "python3")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")

    # --- RPM ---
    set(CPACK_RPM_PACKAGE_SUMMARY "Lightweight Audio Player")
    set(CPACK_RPM_PACKAGE_GROUP "Applications/Multimedia")
    set(CPACK_RPM_PACKAGE_LICENSE "GPLv3") 
    set(CPACK_RPM_PACKAGE_AUTOREQ ON)   
    set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtbase, portaudio, taglib, libsndfile, mpg123, python3")
endif()

include(CPack)