include_guard()

include("cmake/cabbage_version.cmake")

message("")
message("-----------------------------------")
message("Configuring ${CABBAGE_BUILD_TARGET}")
message("-----------------------------------")

# Set the base directory for fetched dependencies so all Cabbage build targets use the same dependencies.
set(FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/../_deps")
cmake_path(ABSOLUTE_PATH FETCHCONTENT_BASE_DIR BASE_DIRECTORY "${CMAKE_BINARY_DIR}" NORMALIZE)
message(DEBUG "FETCHCONTENT_BASE_DIR = ${FETCHCONTENT_BASE_DIR}")

# Initialize vcpkg. Must be done before project() call.
include("cmake/init_vcpkg.cmake")

project(${CABBAGE_PROJECT_NAME} VERSION ${CABBAGE_VERSION})

find_package(ixwebsocket CONFIG REQUIRED)

if(CabbageApp STREQUAL "${CABBAGE_BUILD_TARGET}")
    set(CABBAGE_BUILD_TARGET_TYPE app)
    set(CABBAGE_BUILD_APP_TYPE vscode_service)
elseif(CabbageStandaloneApp STREQUAL "${CABBAGE_BUILD_TARGET}")
    set(CABBAGE_BUILD_TARGET_TYPE app)
    set(CABBAGE_BUILD_APP_TYPE standalone)
elseif(CabbageAUv2Effect STREQUAL "${CABBAGE_BUILD_TARGET}")
    set(CABBAGE_BUILD_TARGET_TYPE plugin)
    set(CABBAGE_BUILD_PLUGIN_TYPE auv2)
    set(CABBAGE_BUILD_PLUGIN_PROCESSING_TYPE effect)
elseif(CabbageAUv2Synth STREQUAL "${CABBAGE_BUILD_TARGET}")
    set(CABBAGE_BUILD_TARGET_TYPE plugin)
    set(CABBAGE_BUILD_PLUGIN_TYPE auv2)
    set(CABBAGE_BUILD_PLUGIN_PROCESSING_TYPE effect)
elseif(CabbageVST3Effect STREQUAL "${CABBAGE_BUILD_TARGET}")
    set(CABBAGE_BUILD_TARGET_TYPE plugin)
    set(CABBAGE_BUILD_PLUGIN_TYPE vst3)
    set(CABBAGE_BUILD_PLUGIN_PROCESSING_TYPE effect)
elseif(CabbageVST3Synth STREQUAL "${CABBAGE_BUILD_TARGET}")
    set(CABBAGE_BUILD_TARGET_TYPE plugin)
    set(CABBAGE_BUILD_PLUGIN_TYPE vst3)
    set(CABBAGE_BUILD_PLUGIN_PROCESSING_TYPE synth)
else()
    message(FATAL_ERROR "Unsupported CABBAGE_BUILD_TARGET: ${CABBAGE_BUILD_TARGET}")
endif()

# Fetch dependencies not provided by vcpkg.
include(cmake/fetch_dependencies.cmake)

if(app STREQUAL "${CABBAGE_BUILD_TARGET_TYPE}")
    message(DEBUG "CABBAGE_BUILD_APP_TYPE = ${CABBAGE_BUILD_APP_TYPE}")
elseif(plugin STREQUAL "${CABBAGE_BUILD_TARGET_TYPE}")
    message(DEBUG "CABBAGE_BUILD_PLUGIN_TYPE = ${CABBAGE_BUILD_PLUGIN_TYPE}")
    message(DEBUG "CABBAGE_BUILD_PLUGIN_PROCESSING_TYPE = ${CABBAGE_BUILD_PLUGIN_PROCESSING_TYPE}")
endif()

# Find packages provided by vcpkg.
find_package(ixwebsocket CONFIG REQUIRED)

# Find Csound.
list(APPEND CMAKE_MODULE_PATH "cmake")
find_package(CSOUND REQUIRED)

enable_language(CXX)

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

# Initialize iPlug2.
include(${iplug2_SOURCE_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

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
    "src/config.h"
    "src/CabbageProcessor.h"
    "src/CabbageProcessor.cpp"
    "src/CabbageParser.h"
    "src/Cabbage.h"
    "src/Cabbage.cpp"
    "src/CabbageUtils.h"
    "src/CabbageServer.cpp"
    "src/CabbageServer.h"
)

if(APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)

    set(CABBAGE_WEBVIEW_SOURCES
        "src/webView/IPlugWebView.h"
        "src/webView/IPlugWebView.mm"
        "src/webView/CabbageEditorDelegate.h"
        "src/webView/CabbageEditorDelegate.mm"
    )

    set(CABBAGE_DEFINES
        OBJC_PREFIX=vCabbage
        SWELL_APP_PREFIX=Swell_vCabbage
        SWELL_COMPILED
    )

    set(CABBAGE_INCLUDE_DIRS
        "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include"
        "${FETCHCONTENT_BASE_DIR}/choc-src/include"
        "${iplug2_SOURCE_DIR}/Dependencies/Build/mac/include"
        "${CMAKE_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/src"
        "${CMAKE_SOURCE_DIR}/src/app"
    )

    # Very possible we don't need some of these - not super familiar with mac frameworks
    set(CABBAGE_LIBRARIES
        "-framework Carbon"
        "-framework AppKit"
        "-framework Metal"
        "-framework MetalKit"
        "-framework WebKit"
        "-framework CoreFoundation"
        ixwebsocket::ixwebsocket
        readerwriterqueue
    )

    # because the file hierarchy needs preserving, I did this the KISS way - but maybe some of the existing iPlug stuff has a more elegant way to do this
    set(CABBAGE_WEB_RESOURCES
        "resources/web/index.html"
        "resources/web/script.js"
        "resources/web/webaudio-controls.js"
    )

    set_source_files_properties("resources/web/index.html" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/web")
    set_source_files_properties("resources/web/script.js" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/web")
    set_source_files_properties("resources/web/webaudio-controls.js" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/web")

    set_source_files_properties("src/webView/IPlugWebView.mm" PROPERTIES COMPILE_OPTIONS -fobjc-arc)

    add_library(_base INTERFACE)

    iplug_target_add(_base INTERFACE
        DEFINE
            CUSTOM_EDITOR="${CMAKE_SOURCE_DIR}/src/webView/CabbageEditorDelegate.h"
            CUSTOM_EDITOR_CLASS=CabbageEditorDelegate
            NO_IGRAPHICS
        FEATURE cxx_std_17
        INCLUDE
            "${CMAKE_SOURCE_DIR}/resources"
            ${CABBAGE_DEFINES}
            ${CABBAGE_INCLUDE_DIRS}
            ${CSOUND_INCLUDE_DIRS}
            ${iplug2_SOURCE_DIR}/Dependencies/Extras/nlohmann
        LINK
            iPlug2_GL2
            ${CABBAGE_LIBRARIES}
            ${CSOUND_FRAMEWORK}
        SOURCE ${CABBAGE_WEB_RESOURCES}
    )
endif()
