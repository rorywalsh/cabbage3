cmake_minimum_required(VERSION 3.26 FATAL_ERROR)

set(BUILD_STANDALONE_APP ON)
set(BUILD_VSCODE_SERVICE_APP OFF)

set(BUILD_AUV2_EFFECT_PLUGIN OFF)
set(BUILD_AUV2_SYNTH_PLUGIN OFF)

set(BUILD_VST3_EFFECT_PLUGIN OFF)
set(BUILD_VST3_SYNTH_PLUGIN OFF)

# Initialize vcpkg. Must be before project() call.
include(cmake/init_vcpkg.cmake)

project(Cabbage VERSION 0.0.98)

# Find packages provided by vcpkg.
find_package(ixwebsocket CONFIG REQUIRED)

# Fetch dependencies not provided by vcpkg.
include(cmake/fetch_dependencies.cmake)

# Find Csound.
list(APPEND CMAKE_MODULE_PATH "cmake")
find_package(CSOUND REQUIRED)

enable_language(CXX)

# Set default build type to Release.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

# Initialize iPlug2.
include(${iplug2_SOURCE_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

# Include iPlug2's APP.cmake here to avoid including it multiple times when iplug_configure_target() is called.
include("${IPLUG2_CMAKE_DIR}/APP.cmake")

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
  "src/CabbageWidgetDescriptors.h"
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

    if(BUILD_STANDALONE_APP)
        add_executable(Cabbage MACOSX_BUNDLE
            ${CABBAGE_OPCODE_SOURCES}
            ${CABBAGE_SOURCES}
            ${CABBAGE_WEBVIEW_SOURCES}
        )

        set(PLUG_NAME "Cabbage" CACHE STRING "" FORCE)

        iplug_target_add(Cabbage PUBLIC
            INCLUDE
                "${CMAKE_SOURCE_DIR}/resources"
            LINK
                _base
                iPlug2_APP
                ${CSOUND_FRAMEWORK}
            RESOURCE ${RESOURCES}
        )

        iplug_configure_target(Cabbage app)

        set_target_properties(Cabbage PROPERTIES XCODE_ATTRIBUTE_PRODUCT_NAME "Cabbage")
    endif()

    if(BUILD_VSCODE_SERVICE_APP)
        add_executable(CabbageVSCodeService MACOSX_BUNDLE
            ${CABBAGE_SOURCES}
            ${CABBAGE_OPCODE_SOURCES}
            ${CABBAGE_WEBVIEW_SOURCES}
        )

        iplug_target_add(CabbageVSCodeService PUBLIC
            INCLUDE
                "${CMAKE_SOURCE_DIR}/resources"
            LINK
                _base
                iPlug2_APP
                ${CSOUND_FRAMEWORK}
            RESOURCE ${RESOURCES}
        )

        iplug_configure_target(CabbageVSCodeService app)
    endif()

    if(BUILD_AUV2_EFFECT_PLUGIN)
        add_library(CabbageAUV2Effect MODULE
            ${CABBAGE_OPCODE_SOURCES}
            ${CABBAGE_SOURCES}
            ${CABBAGE_WEBVIEW_SOURCES}
        )

        set(PLUG_NAME "CabbageAUV2Effect" CACHE STRING "" FORCE)

        iplug_target_add(CabbageAUV2Effect PUBLIC
            INCLUDE
                "${CMAKE_SOURCE_DIR}/resources"
            LINK
                _base
                iPlug2_AUv2
            RESOURCE ${RESOURCES}
        )

        iplug_configure_target(CabbageAUV2Effect auv2)

        set_target_properties(CabbageAUV2Effect PROPERTIES XCODE_ATTRIBUTE_PRODUCT_NAME "CabbageAUV2Effect")
    endif()

    if(BUILD_VST3_EFFECT_PLUGIN)
        add_library(CabbageVST3Effect MODULE
            ${CABBAGE_OPCODE_SOURCES}
            ${CABBAGE_SOURCES}
            ${CABBAGE_WEBVIEW_SOURCES}
        )

        set(PLUG_NAME "CabbageVST3Effect" CACHE STRING "" FORCE)

        iplug_target_add(CabbageVST3Effect PUBLIC
            INCLUDE
                "${CMAKE_SOURCE_DIR}/resources"
            LINK
                _base
                iPlug2_VST3
            RESOURCE ${RESOURCES}
        )

        iplug_configure_target(CabbageVST3Effect vst3)

        set_target_properties(CabbageVST3Effect PROPERTIES XCODE_ATTRIBUTE_PRODUCT_NAME "CabbageVST3Effect")

        # TODO: Make a target that depends on the build .vst3 file.
        add_custom_command(TARGET CabbageVST3Effect POST_BUILD
            COMMAND ${CMAKE_COMMAND} ARGS "-E" "copy_directory" "${CMAKE_BINARY_DIR}/out/CabbageVST3Effect.vst3" "$ENV{HOME}/Library/Audio/Plug-Ins/VST3"
        )
    endif()
endif()
