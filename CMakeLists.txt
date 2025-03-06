cmake_minimum_required(VERSION 3.25)

project(CabbageClap LANGUAGES CXX VERSION 0.1)

if (APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)
    set(CMAKE_OBJC_VISIBILITY_PRESET hidden)
    set(CMAKE_OBJCXX_VISIBILITY_PRESET hidden)
    set(CMAKE_OBJCXX_FLAGS "${CMAKE_OBJCXX_FLAGS} -fobjc-arc")

    # Link against the Cocoa framework
    find_library(COCOA_LIBRARY Cocoa)
endif()

option(COPY_AFTER_BUILD "Install local copy aftger build" TRUE)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CLAP_WRAPPER_DOWNLOAD_DEPENDENCIES TRUE CACHE BOOL "Download Dependencies")
set(CLAP_WRAPPER_OUTPUT_NAME "${PROJECT_NAME}")
set(CLAP_WRAPPER_DONT_ADD_TARGETS TRUE)
include(cmake/Dependencies.cmake)

include_directories(${clap_SOURCE_DIR}/include)
include_directories(${clap-helpers_SOURCE_DIR}/include)

set(PRODUCT_NAME "CabbageClap")

set(SOURCE_FILES
    src/Cumhdach.cpp
    src/Cumhdach.h
    src/CabbageClap/Utils.h
    src/CabbageClap/Plugin.h
    src/CabbageClap/Plugin.cpp
    src/CabbageClap/FactoryImpl.cpp
)

if (APPLE)
    
    list(APPEND SOURCE_FILES
        src/CabbageClap/platform/MacParent.mm
    )
    
endif()

add_library(${PROJECT_NAME}-impl STATIC
    ${SOURCE_FILES}
)


target_include_directories(${PROJECT_NAME}-impl PRIVATE ${clap_SOURCE_DIR}/include)

if(WIN32)
    target_compile_definitions(${PROJECT_NAME}-impl PRIVATE _CRT_SECURE_NO_WARNINGS)    
else()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded" CACHE true)
    target_compile_options(${PROJECT_NAME}-impl PRIVATE -Wall -Wextra -pedantic)
endif()

target_include_directories(${PROJECT_NAME}-impl
    PRIVATE 
        ${choc_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)

if(APPLE)
    target_link_libraries(${PROJECT_NAME}-impl PUBLIC clap clap-helpers ${COCOA_LIBRARY})
endif()



make_clapfirst_plugins(
        TARGET_NAME ${PROJECT_NAME}
        IMPL_TARGET ${PROJECT_NAME}-impl

        OUTPUT_NAME "${PRODUCT_NAME}"

        ENTRY_SOURCE src/CabbageClap/Factory.cpp

        BUNDLE_IDENTIFER "org.yourproject.yourthing"
        BUNDLE_VERSION ${PROJECT_VERSION}

        COPY_AFTER_BUILD ${COPY_AFTER_BUILD}

        PLUGIN_FORMATS CLAP VST3 # AUV2

        ASSET_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME}_assets

)
