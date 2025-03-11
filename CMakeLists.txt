cmake_minimum_required(VERSION 3.25)

# ----------------------------
# Project Configuration
# ----------------------------
project(cabbagePlugin LANGUAGES CXX VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(COPY_AFTER_BUILD "Install local copy after build" TRUE)

# ----------------------------
# Apple-Specific Settings
# ----------------------------
if (APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)
    
    set(CMAKE_OBJC_VISIBILITY_PRESET hidden)
    set(CMAKE_OBJCXX_VISIBILITY_PRESET hidden)
    set(CMAKE_OBJCXX_FLAGS "${CMAKE_OBJCXX_FLAGS} -fobjc-arc")

    # Link against the Cocoa framework
    find_library(COCOA_LIBRARY Cocoa)
endif()

# ----------------------------
# Dependencies
# ----------------------------
set(CLAP_WRAPPER_DOWNLOAD_DEPENDENCIES TRUE CACHE BOOL "Download Dependencies")
set(CLAP_WRAPPER_OUTPUT_NAME "${PROJECT_NAME}")
set(CLAP_WRAPPER_DONT_ADD_TARGETS TRUE)

include(cmake/Dependencies.cmake)
include(cmake/cabs.cmake)

include_directories(
    ${clap_SOURCE_DIR}/include
    ${clap-helpers_SOURCE_DIR}/include
)

# ----------------------------
# Project basename
# ----------------------------
set(PRODUCT_NAME ${PROJECT_NAME})

# ----------------------------
# Source Files
# ----------------------------
set(SOURCE_FILES
    src/TestProcessor.cpp
    src/TestProcessor.h
    src/cabs/CabsProcessor.h
    src/cabs/cabsClap/Plugin.h
    src/cabs/cabsClap/Plugin.cpp
    src/cabs/cabsClap/FactoryImpl.cpp
)

# Apple-Specific Sources
if (APPLE)    
    list(APPEND SOURCE_FILES src/cabs/cabsClap/platform/MacParent.mm)
endif()

# ----------------------------
# Plugin Implementation Library
# ----------------------------
add_library(${PROJECT_NAME}-impl STATIC ${SOURCE_FILES})

# ----------------------------
# Generate CabbagePluginInfo.h header
# ----------------------------
generatePluginInfoHeader(${PROJECT_NAME}-impl
    UNIQUE_ID "your.reversed.domain.name.PluginName"
    NAME "cabbagePlugin"
    VENDOR "CabbageAudio"
    URL "https://cabbageaudio.com"
    MANUAL_URL "https://docs.cabbageaudio.com"
    SUPPORT_URL "https://support.cabbageaudio.com"
    VERSION "1.2.3"
    DESCRIPTION "Awesome Cabbage Plugin"
    FEATURES 
        CLAP_PLUGIN_FEATURE_AUDIO_EFFECT 
        CLAP_PLUGIN_FEATURE_UTILITY 
        CLAP_PLUGIN_FEATURE_STEREO
)

# ----------------------------
# Include Paths
# ----------------------------
target_include_directories(${PROJECT_NAME}-impl PRIVATE 
    ${clap_SOURCE_DIR}/include
    ${choc_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${json_SOURCE_DIR}/include
)

# ----------------------------
# Compiler and Platform-Specific Options
# ----------------------------
if (WIN32)
    target_compile_definitions(${PROJECT_NAME}-impl PRIVATE _CRT_SECURE_NO_WARNINGS)    
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded" CACHE STRING "Multithreaded")
else()
    target_compile_options(${PROJECT_NAME}-impl PRIVATE -Wall -Wextra -pedantic)
endif()

# ----------------------------
# Linking
# ----------------------------
if (APPLE)
    target_link_libraries(${PROJECT_NAME}-impl PUBLIC clap clap-helpers ${COCOA_LIBRARY})
endif()

# ----------------------------
# Plugin Build Configuration
# ----------------------------
make_clapfirst_plugins(
    TARGET_NAME ${PROJECT_NAME}
    IMPL_TARGET ${PROJECT_NAME}-impl
    OUTPUT_NAME "${PRODUCT_NAME}"
    ENTRY_SOURCE src/cabs/cabsClap/Factory.cpp
    BUNDLE_IDENTIFER "org.yourproject.yourthing"
    BUNDLE_VERSION ${PROJECT_VERSION}
    COPY_AFTER_BUILD ${COPY_AFTER_BUILD}
    PLUGIN_FORMATS CLAP VST3 # AUV2
    ASSET_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}_assets
)
