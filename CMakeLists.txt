cmake_minimum_required(VERSION 3.10)
project(PerfectAudioWorks VERSION 0.1.2 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (UNIX)
    find_package(Qt6 REQUIRED COMPONENTS Core Widgets DBus Network)
else()
    find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)
endif()

set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

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
    
    src/PAW_GUI/about_paw_gui.ui
    src/PAW_GUI/about_paw_gui.cpp
    src/PAW_GUI/aboutfile_paw_gui.ui
    src/PAW_GUI/aboutfile_paw_gui.cpp
    src/PAW_GUI/settings_paw_gui.ui
    src/PAW_GUI/settings_paw_gui.cpp
    src/PAW_GUI/main_paw_widget.ui
    src/PAW_GUI/main_paw_widget.cpp
    
    src/PAW_GUI/ScrollingLabel.h
    src/PAW_GUI/ScrollingLabel.cpp

    src/miscellaneous/python_bindings.cpp
    src/miscellaneous/PythonEvent.cpp

    src/PAW_GUI/GlobalLinuxKeys.h
)

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


set(PYBIND11_DIR "${CMAKE_SOURCE_DIR}/extern/pybind11")

if(EXISTS ${PYBIND11_DIR}/CMakeLists.txt)
    add_subdirectory(${PYBIND11_DIR})
    
    target_link_libraries(PAW PRIVATE pybind11::embed)
    
else()
    message(WARNING "Pybind11 submodule not found.")
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


if (UNIX)
target_link_libraries(PAW PRIVATE
    Qt6::Core 
    Qt6::Widgets 
    Qt6::Network
    Qt6::DBus 
    pybind11::embed
    Python3::Python      
    ${PORTAUDIO_TARGET}
    ${TAGLIB_TARGET}
    nlohmann_json::nlohmann_json  
)
else()
target_link_libraries(PAW PRIVATE
    Qt6::Core 
    Qt6::Widgets 
    Qt6::Network
    pybind11::embed
    Python3::Python      
    ${PORTAUDIO_TARGET}
    ${TAGLIB_TARGET}
    nlohmann_json::nlohmann_json  
)
endif()

if (WIN32)
    target_link_libraries(PAW PRIVATE Ws2_32)
    if(MINGW)
        add_definitions(-D_WIN32) 
    endif()
endif()

if(TARGET PAW_python)
    set_target_properties(PAW_python PROPERTIES 
        PREFIX ""
        SUFFIX ".pyd"
        OUTPUT_NAME "PAW_python"
    )
    
    add_dependencies(PAW PAW_python)
endif()

include(GNUInstallDirs)
install(TARGETS PAW RUNTIME DESTINATION bin)

if(WIN32)
    set(KNOWN_GOOD_BIN_DIR "${CMAKE_SOURCE_DIR}/out/install/UCRT64-Release/bin")
    install(DIRECTORY "${KNOWN_GOOD_BIN_DIR}/" DESTINATION bin FILES_MATCHING PATTERN "*.dll")
    install(DIRECTORY "${KNOWN_GOOD_BIN_DIR}/platforms" DESTINATION bin)
    install(DIRECTORY "${KNOWN_GOOD_BIN_DIR}/styles" DESTINATION bin)
    install(DIRECTORY "${KNOWN_GOOD_BIN_DIR}/imageformats" DESTINATION bin)
    set(PY_VER "3.14.0") 
    set(PY_ZIP "python-${PY_VER}-embed-amd64.zip")
    set(PY_URL "https://www.python.org/ftp/python/${PY_VER}/${PY_ZIP}")
    set(PY_DIR "${CMAKE_BINARY_DIR}/python_embed")
    
    if(NOT EXISTS "${PY_DIR}")
        file(DOWNLOAD "${PY_URL}" "${CMAKE_BINARY_DIR}/${PY_ZIP}")
        file(ARCHIVE_EXTRACT INPUT "${CMAKE_BINARY_DIR}/${PY_ZIP}" DESTINATION "${PY_DIR}")
    endif()
    
    install(DIRECTORY "${PY_DIR}/" 
            DESTINATION bin 
            FILES_MATCHING PATTERN "*.dll" PATTERN "*.zip" PATTERN "*.pyd")
endif()

if(UNIX AND NOT APPLE)
    install(FILES "${CMAKE_SOURCE_DIR}/paw.desktop" DESTINATION share/applications)
    install(FILES "${CMAKE_SOURCE_DIR}/src/assets/icon_64.png" DESTINATION share/icons/hicolor/64x64/apps RENAME paw.png)
    install(FILES "${CMAKE_SOURCE_DIR}/src/assets/icon_256.png" DESTINATION share/icons/hicolor/256x256/apps RENAME paw.png)
endif()

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
    set(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\PerfectAudioWorks.lnk' '$INSTDIR\\\\bin\\\\PAW.exe'")
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "CreateShortCut '$DESKTOP\\\\PerfectAudioWorks.lnk' '$INSTDIR\\\\bin\\\\PAW.exe'")
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "Delete '$DESKTOP\\\\PerfectAudioWorks.lnk'\\n  Delete '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\PerfectAudioWorks.lnk'")
endif()

if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "CapitanMurasa")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON) 
    set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "CapitanMurasa vladislavkolodiy45@gmail.com")
    set(CPACK_PACKAGE_VENDOR "CapitanMurasa")
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/CapitanMurasa/PerfectAudioWorks")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE") 
endif()

include(CPack)