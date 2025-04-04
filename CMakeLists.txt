# Minimum CMake version required
cmake_minimum_required(VERSION 3.25)

# Set the version of the Cabbage project
set(CABBAGE_VERSION 0.0.100)

# Define common base directory for all dependency downloads and builds
set(FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/_deps" CACHE PATH "Base directory for downloaded dependencies")

# -----------------------------
# Set compiler standard and build options for CLAP wrapper
# -----------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ----------------------------
# If no specific build target is provided, build all targets
# ----------------------------
if(NOT DEFINED CABBAGE_BUILD_TARGET)

    set(CABBAGE_BUILD_TARGETS CabbageApp CabbagePluginEffect CabbagePluginSynth)
    message(STATUS "No CABBAGE_BUILD_TARGET specified. Building all targets.")

    # Include shared dependencies configuration
    include(${CMAKE_SOURCE_DIR}/cmake/Dependencies.cmake)

    # Configure and build each target separately
    foreach(target IN LISTS CABBAGE_BUILD_TARGETS)
        message(STATUS "Configuring build for target: ${target}")

        file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${target})

        # Run CMake configuration for the current target - this needs to be a little verbose for GH actions
        execute_process(
            COMMAND ${CMAKE_COMMAND} 
                -DCABBAGE_BUILD_TARGET=${target} 
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DFETCHCONTENT_BASE_DIR=${FETCHCONTENT_BASE_DIR}
                -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                -DCMAKE_OBJC_COMPILER=${CMAKE_OBJC_COMPILER}
                -DCMAKE_OBJCXX_COMPILER=${CMAKE_OBJCXX_COMPILER}
                -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}
                -S ${CMAKE_SOURCE_DIR} 
                -G ${CMAKE_GENERATOR}
                -B ${CMAKE_BINARY_DIR}/${target}
            RESULT_VARIABLE result
        )

        if(NOT result EQUAL 0)
            message(FATAL_ERROR "Failed to configure ${target}")
        endif()

        # Build the target
        if(BUILD)
            message(STATUS "Building target: ${target}")
            execute_process(
                COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/${target}
                RESULT_VARIABLE result
            )
        endif()

    endforeach()

    # Exit after all builds are done
    return()
endif()

# ----------------------------
# Determine languages used based on platform
# ----------------------------
if(APPLE)
    set(BUILD_LANGUAGES CXX OBJCXX)
else()
    set(BUILD_LANGUAGES CXX)
endif()

# ----------------------------
# Setup project according to specified build target
# ----------------------------
message(STATUS "Setting build target to ${CABBAGE_BUILD_TARGET}")
if (CabbageApp STREQUAL "${CABBAGE_BUILD_TARGET}")
    project(CabbageApp LANGUAGES ${BUILD_LANGUAGES} VERSION 0.1)
elseif (CabbagePluginEffect STREQUAL "${CABBAGE_BUILD_TARGET}")
    project(CabbagePluginEffect LANGUAGES ${BUILD_LANGUAGES} VERSION 0.1)
elseif (CabbagePluginSynth STREQUAL "${CABBAGE_BUILD_TARGET}")
    project(CabbagePluginSynth LANGUAGES ${BUILD_LANGUAGES} VERSION 0.1)
endif()

# ----------------------------
# Set static MSVC runtime for Windows builds
# ----------------------------
if (NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
  if (WIN32)
    message(STATUS "CMAKE_MSVC_RUNTIME_LIBRARY not defined. Setting to static link")
  endif()
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# ----------------------------
# Include dependencies and modules
# ----------------------------
include(${CMAKE_SOURCE_DIR}/cmake/Dependencies.cmake)
include(${lattice_SOURCE_DIR}/cmake/Dependencies.cmake)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(CSOUND REQUIRED)

set(LATTICE_ROOT_DIR "${lattice_SOURCE_DIR}")
include(${LATTICE_ROOT_DIR}/cmake/lattice.cmake)

# ----------------------------
# Set project name and output name
# ----------------------------
set(PROJECT_NAME ${CABBAGE_BUILD_TARGET})
set(CLAP_WRAPPER_OUTPUT_NAME ${PROJECT_NAME})
add_compile_definitions(${CABBAGE_BUILD_TARGET})

# ----------------------------
# Define opcode-related source files
# ----------------------------
set(CABBAGE_OPCODE_SOURCES
    "${CMAKE_SOURCE_DIR}/src/opcodes/CabbageGetOpcodes.h"
    "${CMAKE_SOURCE_DIR}/src/opcodes/CabbageGetOpcodes.cpp"
    "${CMAKE_SOURCE_DIR}/src/opcodes/CabbageSetOpcodes.h"
    "${CMAKE_SOURCE_DIR}/src/opcodes/CabbageSetOpcodes.cpp"
    "${CMAKE_SOURCE_DIR}/src/opcodes/CabbageProfilerOpcodes.h"
    "${CMAKE_SOURCE_DIR}/src/opcodes/CabbageProfilerOpcodes.cpp"
    "${CMAKE_SOURCE_DIR}/src/opcodes/CabbageOpcodes.h"
)

# ----------------------------
# Define core source files
# ----------------------------
set(CABBAGE_SOURCES
    "${CMAKE_SOURCE_DIR}/src/CabbageProcessor.cpp"
    "${CMAKE_SOURCE_DIR}/src/CabbageProcessor.h"
    "${CMAKE_SOURCE_DIR}/src/Cabbage.cpp"
    "${CMAKE_SOURCE_DIR}/src/Cabbage.h"
    "${CMAKE_SOURCE_DIR}/src/CabbageColours.h"
    "${CMAKE_SOURCE_DIR}/src/CabbageParser.cpp"
    "${CMAKE_SOURCE_DIR}/src/CabbageParser.h"
    "${CMAKE_SOURCE_DIR}/src/CabbageUtils.cpp"
    "${CMAKE_SOURCE_DIR}/src/CabbageUtils.h"
    "${CMAKE_SOURCE_DIR}/src/CabbagePluginEffectInfo.h"
)

# ----------------------------
# Combine all source files
# ----------------------------
set(SOURCE_FILES
    ${CABBAGE_SOURCES}
    ${CABBAGE_OPCODE_SOURCES}
    ${LATTICE_SOURCE_FILES}
)

# ----------------------------
# Organize source files into groups
# ----------------------------
source_group("Cabbage Source" FILES ${CABBAGE_SOURCES})
source_group("Cabbage Opcodes" FILES ${CABBAGE_OPCODE_SOURCES})
source_group("Lattice Source" FILES ${LATTICE_SOURCE_FILES})

# ----------------------------
# Build Executable Target (App)
# ----------------------------
if (CabbageApp STREQUAL "${CABBAGE_BUILD_TARGET}")
    set(CABBAGE_SERVICE_APP_SOURCES
        "${CMAKE_SOURCE_DIR}/src/CabbageAudioApp/main.cpp"
        "${CMAKE_SOURCE_DIR}/src/CabbageAudioApp/CabbageAudioApp.cpp"
        "${CMAKE_SOURCE_DIR}/src/CabbageAudioApp/CabbageAudioApp.h"
    )
    list(APPEND SOURCE_FILES ${CABBAGE_SERVICE_APP_SOURCES})
    source_group("Cabbage Service App" FILES ${CABBAGE_SERVICE_APP_SOURCES})

    add_executable(${PROJECT_NAME}  ${SOURCE_FILES})

    # Set include paths for external dependencies
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

    target_compile_definitions(${PROJECT_NAME} PRIVATE 
        PLUGIN_INFO_HEADER="${CMAKE_SOURCE_DIR}/src/CabbagePluginEffectInfo.h"
        PRODUCT_NAME="${PROJECT_NAME}"
        "${CABBAGE_BUILD_TARGET}"
    )

    # Link libraries
    target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/_deps/rtaudio-build)
    target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/_deps/rtmidi-build)
    target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/_deps/ixwebsocket-build)

    if (APPLE)
        target_link_libraries(${PROJECT_NAME} PRIVATE rtaudio rtmidi ixwebsocket ${CSOUND_FRAMEWORK})
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE rtaudio rtmidi ixwebsocket ${CSOUND_LIBRARY})
    endif()

    # Use C++17 for the executable
    set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
    )

# ----------------------------
# Build Plugin Library Target
# ----------------------------
else()
    add_library(${PROJECT_NAME}-impl STATIC ${SOURCE_FILES})

    target_compile_definitions(${PROJECT_NAME}-impl PRIVATE 
        PLUGIN_INFO_HEADER="${CMAKE_SOURCE_DIR}/src/CabbagePluginEffectInfo.h"
        PRODUCT_NAME="${PROJECT_NAME}"
        "${CABBAGE_BUILD_TARGET}"
    )

    target_include_directories(${PROJECT_NAME}-impl PRIVATE 
        ${lattice_SOURCE_DIR}/src
        ${choc_SOURCE_DIR}
        ${readerwriterqueue_SOURCE_DIR}
        ${LATTICE_INCLUDE_DIRS}
        ${CSOUND_INCLUDE_DIR}
    )

    # Platform-specific options
    if (WIN32)
        target_compile_definitions(${PROJECT_NAME}-impl PRIVATE _CRT_SECURE_NO_WARNINGS)    
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded" CACHE STRING "Multithreaded")
    else()
        target_compile_options(${PROJECT_NAME}-impl PRIVATE -Wall -Wextra -pedantic)
    endif()

    # Link required libraries
    if (APPLE)
        target_link_libraries(${PROJECT_NAME}-impl PUBLIC clap clap-helpers ${COCOA_LIBRARY} ${CSOUND_FRAMEWORK})
    else()
        target_link_libraries(${PROJECT_NAME}-impl PUBLIC ${CSOUND_LIBRARY})
    endif()

    # Generate plugin formats
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
