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

# --- CMake Options for Decoders ---
option(ENABLE_SNDFILE "Enable libsndfile support" ON)
option(ENABLE_MPG123 "Enable mpg123 support" ON)
option(ENABLE_FAAD    "Enable FAAD2 (AAC/M4A) support" ON) # <--- NEW OPTION

# --- Source Files ---
# Note: Listing .h files here ensures they are handled by AUTOMOC and appear in IDEs
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

# --- CRITICAL FIX: Include Directories ---
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
    message(FATAL_ERROR "PortAudio not found. Install 'portaudio19-dev' or 'mingw-w64-ucrt-x86_64-portaudio'")
endif()

# 2. TagLib (Bundled)
set(TAGLIB_DIR "${CMAKE_SOURCE_DIR}/extern/taglib")
if(EXISTS ${TAGLIB_DIR}/CMakeLists.txt)
    message(STATUS "Using bundled TagLib from ${TAGLIB_DIR}")
    add_subdirectory(${TAGLIB_DIR} EXCLUDE_FROM_ALL)
    target_include_directories(PAW PRIVATE ${TAGLIB_DIR}/taglib)
    set(TAGLIB_TARGET tag)
    if(WIN32 OR MINGW)
        target_compile_definitions(tag PRIVATE TAGLIB_WITH_WINDOWS_ENCODING)
    endif()
else()
    message(FATAL_ERROR "TagLib source not found. Clone it under extern/taglib.")
endif()

# 3. JSON (Bundled)
set(JSON_DIR "${CMAKE_SOURCE_DIR}/extern/json")
if(EXISTS ${JSON_DIR}/CMakeLists.txt)
    message(STATUS "Using bundled nlohmann_json from ${JSON_DIR}")
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    add_subdirectory(${JSON_DIR} EXCLUDE_FROM_ALL)
else()
    message(FATAL_ERROR "nlohmann_json source not found. Clone it under extern/json.")
endif()

# 4. libsndfile (Optional)
if(ENABLE_SNDFILE)
    find_path(SNDFILE_INCLUDE_DIR sndfile.h)
    find_library(SNDFILE_LIB NAMES sndfile)
    if(SNDFILE_INCLUDE_DIR AND SNDFILE_LIB)
        message(STATUS "libsndfile enabled")
        target_compile_definitions(PAW PRIVATE ENABLE_SNDFILE)
        target_include_directories(PAW PRIVATE ${SNDFILE_INCLUDE_DIR})
        target_sources(PAW PRIVATE src/AudioPharser/libsndfiledecoder.c)
        target_link_libraries(PAW PRIVATE ${SNDFILE_LIB})
    endif()
endif()

# 5. mpg123 (Optional)
if(ENABLE_MPG123)
    find_path(MPG123_INCLUDE_DIR mpg123.h)
    find_library(MPG123_LIB NAMES mpg123)
    if(MPG123_INCLUDE_DIR AND MPG123_LIB)
        message(STATUS "mpg123 enabled")
        target_compile_definitions(PAW PRIVATE ENABLE_MPG123)
        target_include_directories(PAW PRIVATE ${MPG123_INCLUDE_DIR})
        target_sources(PAW PRIVATE src/AudioPharser/mpg123decoder.c)
        target_link_libraries(PAW PRIVATE ${MPG123_LIB})
    endif()
endif()

if(ENABLE_FAAD)
    set(FAAD_DIR "${CMAKE_SOURCE_DIR}/extern/faad2")
    
    if(EXISTS "${FAAD_DIR}/CMakeLists.txt")
        message(STATUS "Building FAAD2 and mp4ff from submodule")


        add_library(mp4ff STATIC
            ${FAAD_DIR}/common/mp4ff/mp4ff.c
            ${FAAD_DIR}/common/mp4ff/mp4ff_int_types.h
            ${FAAD_DIR}/common/mp4ff/mp4ff.h
        )
        target_include_directories(mp4ff PUBLIC ${FAAD_DIR}/common/mp4ff)

        set(FAAD_BUILD_BINARIES OFF CACHE INTERNAL "")
        set(FAAD_BUILD_SHARED_LIBS OFF CACHE INTERNAL "") 
        
        add_subdirectory(${FAAD_DIR} EXCLUDE_FROM_ALL)
        

        if(TARGET libfaad)
            set(FAAD_TARGET libfaad)
        elseif(TARGET faad)
            set(FAAD_TARGET faad)
        else()
            message(FATAL_ERROR "Could not find faad target in submodule")
        endif()
        
        target_compile_definitions(PAW PRIVATE ENABLE_FAAD)
        
        target_sources(PAW PRIVATE src/AudioPharser/faaddecoder.c)
        
        target_link_libraries(PAW PRIVATE ${FAAD_TARGET} mp4ff)
        
        target_include_directories(PAW PRIVATE ${FAAD_DIR}/include)

    else()
        message(WARNING "FAAD2 submodule not found at ${FAAD_DIR}. Run: git submodule update --init")
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

# --- Installation & Packaging ---
include(GNUInstallDirs)

install(TARGETS PAW
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION .
)

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