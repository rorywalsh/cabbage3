# Minimum CMake version required
cmake_minimum_required(VERSION 3.25)
cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0149 NEW)

# Set the version of the Cabbage project
set(CABBAGE_VERSION 0.0.100)

# Enforce Xcode generator on macOS
if(APPLE)
    set(VST3_FORMATS CLAP VST3)
    set(AU_FORMATS CLAP AUV2)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS deployment target")
    endif()
    if(NOT CMAKE_GENERATOR STREQUAL "Xcode")
        message(FATAL_ERROR "Xcode generator is required on macOS. Please use: cmake -G Xcode ...")
    endif()
else()
    set(VST3_FORMATS CLAP VST3)
    set(AU_FORMATS)  # Empty for non-Apple platforms
endif()


# Build options
option(CABBAGE_ENABLE_TESTS "Build tests in addition to main app" OFF)
option(USER_LIB "Output AU plugins directly to ~/Library/Audio/Plug-Ins/Components without Debug subdirectory" OFF)
option(CABBAGE_PRO_BUILD "Build Cabbage Pro version with encryption support" OFF)

# Define common base directory for all dependency downloads and builds
set(FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/_deps" CACHE PATH "Base directory for downloaded dependencies")

# Include shared dependencies configuration
# ----------------------------
# Set dynamic MSVC runtime for Windows builds to match Csound
# ----------------------------
if (NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
  if (WIN32)
    message(STATUS "CMAKE_MSVC_RUNTIME_LIBRARY not defined. Setting to dynamic link")
  endif()
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE STRING "MSVC Runtime Library" FORCE)
endif()



# -----------------------------
# Set compiler standard and build options for CLAP wrapper
# -----------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # Optional: use standard-compliant mode

option(BUILD_TARGETS "Build targets after configuration" OFF)
option(CABBAGE_ENABLE_TESTS "Build Cabbage unit tests" OFF)

# ----------------------------
# If no specific build target is provided, build all targets
# ----------------------------
if(NOT DEFINED CABBAGE_BUILD_TARGET)

    set(CABBAGE_BUILD_TARGETS CabbageApp CabbagePluginEffect CabbagePluginSynth CabbagePluginEffectAUv2 CabbagePluginSynthAUv2 CabbageTests)
    message(STATUS "No CABBAGE_BUILD_TARGET specified. Building all targets.")



    # Configure and build each target separately
    foreach(target IN LISTS CABBAGE_BUILD_TARGETS)
        # Skip AU targets on non-Apple platforms
        if((target STREQUAL "CabbagePluginEffectAUv2" OR target STREQUAL "CabbagePluginSynthAUv2") AND NOT APPLE)
            message(STATUS "Skipping ${target} on non-Apple platform")
            continue()
        endif()
        # Skip tests unless explicitly enabled
        if(target STREQUAL "CabbageTests" AND NOT CABBAGE_ENABLE_TESTS)
            message(STATUS "Skipping ${target} (tests not enabled)")
            continue()
        endif()

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
                -DCMAKE_CXX_STANDARD=20
                -DCMAKE_CXX_STANDARD_REQUIRED=ON
                -DCABBAGE_PRO_BUILD=${CABBAGE_PRO_BUILD}
                -DPRO_MASTER_KEY=${PRO_MASTER_KEY}
                -DPRO_COMPANY_NAME=${PRO_COMPANY_NAME}
                -S ${CMAKE_SOURCE_DIR}
                -G ${CMAKE_GENERATOR}
                -B ${CMAKE_BINARY_DIR}/${target}
            RESULT_VARIABLE result
        )

        if(NOT result EQUAL 0)
            message(FATAL_ERROR "Failed to configure ${target}")
        endif()

        # Build the target
        if(BUILD_ALL_TARGETS AND BUILD_ALL_TARGETS STREQUAL "True" OR BUILD_ALL_TARGETS)
            message(STATUS "Building target: ${target}")

            if(NOT CMAKE_BUILD_TYPE)
                set(CMAKE_BUILD_TYPE "Release")
            endif()

            execute_process(
                COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/${target} --config ${CMAKE_BUILD_TYPE}
                RESULT_VARIABLE result
            )
            if(NOT result EQUAL 0)
                message(FATAL_ERROR "Failed to build ${target}")
            endif()
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
    set(CabbageAUInstrumentType aufx)
    set(FORMATS ${VST3_FORMATS})
elseif (CabbagePluginSynth STREQUAL "${CABBAGE_BUILD_TARGET}")
    project(CabbagePluginSynth LANGUAGES ${BUILD_LANGUAGES} VERSION 0.1)
    set(CabbageAUInstrumentType aumi)
    set(FORMATS ${VST3_FORMATS})
elseif (CabbagePluginEffectAUv2 STREQUAL "${CABBAGE_BUILD_TARGET}")
    project(CabbagePluginEffectAUv2 LANGUAGES ${BUILD_LANGUAGES} VERSION 0.1)
    set(CabbageAUInstrumentType aufx)
    set(FORMATS ${AU_FORMATS})
elseif (CabbagePluginSynthAUv2 STREQUAL "${CABBAGE_BUILD_TARGET}")
    project(CabbagePluginSynthAUv2 LANGUAGES ${BUILD_LANGUAGES} VERSION 0.1)
    set(CabbageAUInstrumentType aumi)
    set(FORMATS ${AU_FORMATS})
elseif (CabbageTests STREQUAL "${CABBAGE_BUILD_TARGET}")
    project(CabbageTests LANGUAGES ${BUILD_LANGUAGES} VERSION 0.1)
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
    "${CMAKE_SOURCE_DIR}/src/opcodes/CabbageCreateOpcode.h"
    "${CMAKE_SOURCE_DIR}/src/opcodes/CabbageCreateOpcode.cpp"
    "${CMAKE_SOURCE_DIR}/src/opcodes/CabbageOpcodes.h"
    "${CMAKE_SOURCE_DIR}/src/opcodes/CabbageStateOpcodes.cpp"
    "${CMAKE_SOURCE_DIR}/src/opcodes/CabbageStateOpcodes.h"
    "${CMAKE_SOURCE_DIR}/src/opcodes/CabbageFileOpcodes.cpp"
    "${CMAKE_SOURCE_DIR}/src/opcodes/CabbageFileOpcodes.h"
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
    "${CMAKE_SOURCE_DIR}/src/CabbagePluginInfo.h"
)

# Add platform-specific file for browseForFile
if(APPLE)
    list(APPEND CABBAGE_SOURCES "${CMAKE_SOURCE_DIR}/src/CabbageUtils_mac.mm")
elseif(WIN32)
    list(APPEND CABBAGE_SOURCES "${CMAKE_SOURCE_DIR}/src/CabbageUtils_win.cpp")
else()
    list(APPEND CABBAGE_SOURCES "${CMAKE_SOURCE_DIR}/src/CabbageUtils_linux.cpp")
endif()

# Add decryption implementation for Pro builds
if(CABBAGE_PRO_BUILD)
    list(APPEND CABBAGE_SOURCES "${CMAKE_SOURCE_DIR}/src/decrypt_impl.cpp")
endif()

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
# Set include directories for all targets
# ----------------------------
include_directories(
    ${lattice_SOURCE_DIR}/src
    ${choc_SOURCE_DIR}
    ${catch2_SOURCE_DIR}
    ${readerwriterqueue_SOURCE_DIR}
    ${LATTICE_INCLUDE_DIRS}
    ${CSOUND_INCLUDE_DIR}
)

# ----------------------------
# Build Executable Target (App)
# ----------------------------
# In order to debug CabbageApp without vscode - start with it the following command:
# --file"path_to_file.csd" --portNumber=NNNN --startTestServer=True

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
        ${rtaudio_SOURCE_DIR}
        ${rtmidi_SOURCE_DIR}
    )


    target_compile_definitions(${PROJECT_NAME} PRIVATE
        PLUGIN_INFO_HEADER="${CMAKE_SOURCE_DIR}/src/CabbagePluginInfo.h"
        PRODUCT_NAME="${PROJECT_NAME}"
        USE_DOUBLE=1
        "${CABBAGE_BUILD_TARGET}"
    )

    # Add CabbagePro definition if building Pro version
    if(CABBAGE_PRO_BUILD)
        target_compile_definitions(${PROJECT_NAME} PRIVATE
            CabbagePro
            PRO_MASTER_KEY="${PRO_MASTER_KEY}"
            PRO_COMPANY_NAME="${PRO_COMPANY_NAME}"
        )
    endif()

    if(WIN32)
        target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32_LEAN_AND_MEAN)
    endif()

    # Link libraries
    target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/_deps/rtaudio-build)
    target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/_deps/rtmidi-build)

    if (APPLE)
        target_link_libraries(${PROJECT_NAME} PRIVATE rtaudio rtmidi ${CSOUND_FRAMEWORK} "-framework Cocoa")
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE rtaudio rtmidi ${CSOUND_LIBRARY})
    endif()

    # Use C++17 for the executable
    set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
    )

endif()

# ----------------------------
# Build Test Target (CabbageTests)
# ----------------------------
if (CabbageTests STREQUAL "${CABBAGE_BUILD_TARGET}")
    enable_testing()

    # Create test-specific source list (exclude platform-specific files)
    set(CABBAGE_TEST_SOURCES
        "${CMAKE_SOURCE_DIR}/src/CabbageProcessor.cpp"
        "${CMAKE_SOURCE_DIR}/src/CabbageProcessor.h"
        "${CMAKE_SOURCE_DIR}/src/Cabbage.cpp"
        "${CMAKE_SOURCE_DIR}/src/Cabbage.h"
        "${CMAKE_SOURCE_DIR}/src/CabbageColours.h"
        "${CMAKE_SOURCE_DIR}/src/CabbageParser.cpp"
        "${CMAKE_SOURCE_DIR}/src/CabbageParser.h"
        "${CMAKE_SOURCE_DIR}/src/CabbageUtils.cpp"
        "${CMAKE_SOURCE_DIR}/src/CabbageUtils.h"
        "${CMAKE_SOURCE_DIR}/src/CabbagePluginInfo.h"
    )

    # Create test executable
    add_executable(${PROJECT_NAME}
        "${CMAKE_SOURCE_DIR}/tests/CabbageAppTests.cpp"
        "${CMAKE_SOURCE_DIR}/tests/TestCsdFiles.h"
        ${CABBAGE_TEST_SOURCES}
        ${CABBAGE_OPCODE_SOURCES}
        ${LATTICE_SOURCE_FILES}
        "${CMAKE_SOURCE_DIR}/src/CabbageAudioApp/CabbageAudioApp.cpp"
        "${CMAKE_SOURCE_DIR}/src/CabbageAudioApp/CabbageAudioApp.h"
        "${CMAKE_SOURCE_DIR}/tests/FileBrowseStub.cpp"
    )
    
    # Set include paths for test target
    target_include_directories(${PROJECT_NAME} PRIVATE 
        ${rtaudio_SOURCE_DIR}
        ${rtmidi_SOURCE_DIR}
    )
    
    # Set compile definitions for test target
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        PLUGIN_INFO_HEADER="${CMAKE_SOURCE_DIR}/src/CabbagePluginInfo.h"
        PRODUCT_NAME="CabbageApp"
        USE_DOUBLE=1
        CabbageTests
        CabbageApp
    )

    # Add CabbagePro definition if building Pro version
    if(CABBAGE_PRO_BUILD)
        target_compile_definitions(${PROJECT_NAME} PRIVATE
            CabbagePro
            PRO_MASTER_KEY="${PRO_MASTER_KEY}"
            PRO_COMPANY_NAME="${PRO_COMPANY_NAME}"
        )
    endif()
    
    if(WIN32)
        target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32_LEAN_AND_MEAN)
    endif()
    
    # Link libraries for test target
    target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/_deps/rtaudio-build)
    target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/_deps/rtmidi-build)
    
    if (APPLE)
        # Ensure Cocoa is linked for UI code (NSOpenPanel etc.)
        target_link_libraries(${PROJECT_NAME} PRIVATE rtaudio rtmidi ${CSOUND_FRAMEWORK} "-framework Cocoa" Catch2::Catch2WithMain)
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE rtaudio rtmidi ${CSOUND_LIBRARY} Catch2::Catch2WithMain)
    endif()
    
    # Set C++ standard for test target
    set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
    )
    
    # Add test to CTest
    add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
endif()

# ----------------------------
# Build Plugin Library Target
# ----------------------------
if (NOT (CabbageApp STREQUAL "${CABBAGE_BUILD_TARGET}") AND NOT (CabbageTests STREQUAL "${CABBAGE_BUILD_TARGET}"))
    add_library(${PROJECT_NAME}-impl STATIC ${SOURCE_FILES})

    target_compile_definitions(${PROJECT_NAME}-impl PRIVATE
        PLUGIN_INFO_HEADER="${CMAKE_SOURCE_DIR}/src/CabbagePluginInfo.h"
        PRODUCT_NAME="${PROJECT_NAME}"
        "${CABBAGE_BUILD_TARGET}"
    )

    # Add CabbagePro definition if building Pro version
    if(CABBAGE_PRO_BUILD)
        target_compile_definitions(${PROJECT_NAME}-impl PRIVATE
            CabbagePro
            PRO_MASTER_KEY="${PRO_MASTER_KEY}"
        )
    endif()

    # Platform-specific options
    if (WIN32)
        target_compile_definitions(${PROJECT_NAME}-impl PRIVATE _CRT_SECURE_NO_WARNINGS)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded" CACHE STRING "Multithreaded")
    else()
        target_compile_options(${PROJECT_NAME}-impl PRIVATE -Wall -Wextra -pedantic)
    endif()

    # Link required libraries
    if (APPLE)
        # Add WebKit and Cocoa frameworks for choc::ui::WebView support
        target_link_libraries(${PROJECT_NAME}-impl PUBLIC clap clap-helpers "-framework Cocoa" "-framework WebKit" ${CSOUND_FRAMEWORK})
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
        PLUGIN_FORMATS ${FORMATS}
        ASSET_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}_assets
        AUV2_INSTRUMENT_TYPE "${CabbageAUInstrumentType}"
        # STANDALONE_CONFIGURATIONS
        #     standalone "${PROJECT_NAME}" "your.reversed.domain.name.${PROJECT_NAME}"
    )

    if(TARGET ${PROJECT_NAME}_auv2)
        # If USER_LIB flag is set, override output directories to place plugins directly 
        # in ~/Library/Audio/Plug-Ins/Components without Debug/Release subdirectories
        if(USER_LIB)
            set_target_properties(${PROJECT_NAME}_auv2 PROPERTIES
                LIBRARY_OUTPUT_DIRECTORY_DEBUG /Users/rwalsh/Library/Audio/Plug-Ins/Components
                LIBRARY_OUTPUT_DIRECTORY_RELEASE /Users/rwalsh/Library/Audio/Plug-Ins/Components
                LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO /Users/rwalsh/Library/Audio/Plug-Ins/Components
                LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL /Users/rwalsh/Library/Audio/Plug-Ins/Components
            )
        endif()
    endif()

endif()