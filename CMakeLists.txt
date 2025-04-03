
cmake_minimum_required(VERSION 3.25)
set(CABBAGE_VERSION 0.0.100)

# ----------------------------
# Set default project to build
# ----------------------------
if(NOT CABBAGE_BUILD_TARGET)
    set(CABBAGE_BUILD_TARGET "CabbagePluginEffect" CACHE STRING "Setting build target to plugin effect by default")
endif()

# ----------------------------
# Project Configuration
# ----------------------------
message(STATUS "Setting build target to ${CABBAGE_BUILD_TARGET}")
if (CabbageApp STREQUAL "${CABBAGE_BUILD_TARGET}")
    project(CabbageApp LANGUAGES CXX VERSION 0.1)
elseif (CabbagePluginEffect STREQUAL "${CABBAGE_BUILD_TARGET}")
    project(CabbagePluginEffect LANGUAGES CXX VERSION 0.1)
elseif (CabbagePluginSynth STREQUAL "${CABBAGE_BUILD_TARGET}")
    project(CabbagePluginSynth LANGUAGES CXX VERSION 0.1)
endif()



if (NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
  if (WIN32)
    message(STATUS "CMAKE_MSVC_RUNTIME_LIBRARY not defined. Setting to static link")
  endif()
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>") # Statically link the MSVC Runtime
endif()

# Add Lattice cmake include
include(${CMAKE_SOURCE_DIR}/cmake/Dependencies.cmake)
include(${lattice_SOURCE_DIR}/cmake/Dependencies.cmake)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(CSOUND REQUIRED)


set(LATTICE_ROOT_DIR "${lattice_SOURCE_DIR}")
# lattice.cmake need to have LATTICE_ROOT_DIR set in advance of use
include(${LATTICE_ROOT_DIR}/cmake/lattice.cmake)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CLAP_WRAPPER_DONT_ADD_TARGETS TRUE)
set(CLAP_WRAPPER_DOWNLOAD_DEPENDENCIES TRUE CACHE BOOL "Download Dependencies")
option(COPY_AFTER_BUILD "Install local copy after build" FALSE)

set(PROJECT_NAME ${CABBAGE_BUILD_TARGET})
set(CLAP_WRAPPER_OUTPUT_NAME ${PROJECT_NAME})
add_compile_definitions(${CABBAGE_BUILD_TARGET})

set(CABBAGE_OPCODE_SOURCES
    "src/opcodes/CabbageGetOpcodes.h"
    "src/opcodes/CabbageGetOpcodes.cpp"
    "src/opcodes/CabbageSetOpcodes.h"
    "src/opcodes/CabbageSetOpcodes.cpp"
    "src/opcodes/CabbageProfilerOpcodes.h"
    "src/opcodes/CabbageProfilerOpcodes.cpp"
    "src/opcodes/CabbageOpcodes.h"
)

set(CABBAGE_SOURCES
    "src/CabbageProcessor.cpp"
    "src/CabbageProcessor.h"
    "src/Cabbage.cpp"
    "src/Cabbage.h"
    "src/CabbageColours.h"
    "src/CabbageParser.cpp"
    "src/CabbageParser.h"
    "src/CabbageUtils.cpp"
    "src/CabbageUtils.h"
)

set(SOURCE_FILES
    ${CABBAGE_SOURCES}
    ${CABBAGE_OPCODE_SOURCES}
    ${LATTICE_SOURCE_FILES} # Include lattice source files
)

source_group("Cabbage Source" FILES ${CABBAGE_SOURCES})
source_group("Cabbage Opcodes" FILES ${CABBAGE_OPCODE_SOURCES})
source_group("Lattice Source" FILES ${LATTICE_SOURCE_FILES})

# ----------------------------
# Conditional Builds - CabbageServiceApp or CabbagePlugin
# ----------------------------
if (CabbageApp STREQUAL "${CABBAGE_BUILD_TARGET}")
    # ----------------------------
    # Build the Service App
    # ----------------------------
    set(CABBAGE_SERVICE_APP_SOURCES
        "src/CabbageAudioApp/main.cpp"
        "src/CabbageAudioApp/CabbageAudioApp.cpp"
        "src/CabbageAudioApp/CabbageAudioApp.h"
    )
    list(APPEND SOURCE_FILES ${CABBAGE_SERVICE_APP_SOURCES})
    source_group("Cabbage Service App" FILES ${CABBAGE_SERVICE_APP_SOURCES})
    
    add_executable(${PROJECT_NAME}  ${SOURCE_FILES})

        # Include Paths
    target_include_directories(${PROJECT_NAME} PRIVATE 
        ${lattice_SOURCE_DIR}/src
        ${choc_SOURCE_DIR}
        ${readerwriterqueue_SOURCE_DIR}
        ${LATTICE_INCLUDE_DIRS}
        ${CSOUND_INCLUDE_DIR}
        ${rtaudio_SOURCE_DIR}
        ${rtmidi_SOURCE_DIR}
        ${ixwebsocket_SOURCE_DIR}
    )

    # Generate plugin header - this will define the project properties
    generate_plugin_header(${PROJECT_NAME}
        OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}  # Output directory for the header
        UNIQUE_ID "your.reversed.domain.name.${PROJECT_NAME}"
        NAME "${PROJECT_NAME}"
        VENDOR "CabbageAudio"
        URL "https://cabbageaudio.com"
        MANUAL_URL "https://docs.cabbageaudio.com"
        SUPPORT_URL "https://support.cabbageaudio.com"
        VERSION "1.0.0"
        DESCRIPTION "${PROJECT_NAME} Plugin"
        FEATURES 
            CLAP_PLUGIN_FEATURE_AUDIO_EFFECT 
            CLAP_PLUGIN_FEATURE_UTILITY 
            CLAP_PLUGIN_FEATURE_STEREO
    )

    # Specify the library directory for RtAudio, RtMidi and ixWebSocket
    target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/_deps/rtaudio-build)
    target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/_deps/rtmidi-build)
    target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/_deps/ixwebsocket-build)


    if (APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE rtaudio rtmidi ixwebsocket ${CSOUND_FRAMEWORK})
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE rtaudio rtmidi ixwebsocket ${CSOUND_LIBRARY})
    endif()

    # Enable C++17 for the executable
    set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
    )
else()
    # ----------------------------
    # Build the Plugin Library
    # ----------------------------

    add_library(${PROJECT_NAME}-impl STATIC ${SOURCE_FILES})

    # Generate plugin header - this will define the project properties
    generate_plugin_header(${PROJECT_NAME}-impl
        OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}  # Output directory for the header
        UNIQUE_ID "your.reversed.domain.name.${PROJECT_NAME}"
        NAME "${PROJECT_NAME}"
        VENDOR "CabbageAudio"
        URL "https://cabbageaudio.com"
        MANUAL_URL "https://docs.cabbageaudio.com"
        SUPPORT_URL "https://support.cabbageaudio.com"
        VERSION "1.0.0"
        DESCRIPTION "${PROJECT_NAME} Plugin"
        FEATURES 
            CLAP_PLUGIN_FEATURE_AUDIO_EFFECT 
            CLAP_PLUGIN_FEATURE_UTILITY 
            CLAP_PLUGIN_FEATURE_STEREO
    )


    # Include Paths
    target_include_directories(${PROJECT_NAME}-impl PRIVATE 
        ${lattice_SOURCE_DIR}/src
        ${choc_SOURCE_DIR}
        ${readerwriterqueue_SOURCE_DIR}
        ${LATTICE_INCLUDE_DIRS}
        ${CSOUND_INCLUDE_DIR}
    )

    # Define a preprocessor macro for the header name
    target_compile_definitions(${PROJECT_NAME}-impl PRIVATE 
        PLUGIN_INFO_HEADER="${PROJECT_NAME}Info.h"
        PRODUCT_NAME="${PROJECT_NAME}"
        "${CABBAGE_BUILD_TARGET}"
    )


    # Compiler and Platform-Specific Options
    if (WIN32)
        target_compile_definitions(${PROJECT_NAME}-impl PRIVATE _CRT_SECURE_NO_WARNINGS)    
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded" CACHE STRING "Multithreaded")
    else()
        target_compile_options(${PROJECT_NAME}-impl PRIVATE -Wall -Wextra -pedantic)
    endif()

    # Linking
    if (APPLE)
        target_link_libraries(${PROJECT_NAME}-impl PUBLIC clap clap-helpers ${COCOA_LIBRARY} ${CSOUND_FRAMEWORK})
    else()
        target_link_libraries(${PROJECT_NAME}-impl PUBLIC ${CSOUND_LIBRARY})
    endif()

    # Plugin Build Configuration
    make_clapfirst_plugins(
        TARGET_NAME ${PROJECT_NAME}
        IMPL_TARGET ${PROJECT_NAME}-impl
        OUTPUT_NAME "${PROJECT_NAME}"
        ENTRY_SOURCE ${LATTICE_ROOT_DIR}/src/lattice/clap/Factory.cpp
        BUNDLE_IDENTIFER "org.yourproject.${PROJECT_NAME}"
        BUNDLE_VERSION ${PROJECT_VERSION}
        COPY_AFTER_BUILD ${COPY_AFTER_BUILD}
        PLUGIN_FORMATS CLAP VST3 # AUV2
        ASSET_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}_assets
        STANDALONE_CONFIGURATIONS
            standalone "${PROJECT_NAME}" "your.reversed.domain.name.${PROJECT_NAME}"
    )

endif()