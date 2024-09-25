cmake_minimum_required(VERSION 3.26 FATAL_ERROR)

set(BUILD_STANDALONE_APP ON)
set(BUILD_VSCODE_SERVICE_APP ON)

set(BUILD_VST3_EFFECT_PLUGIN ON)
set(BUILD_VST3_SYNTH_PLUGIN ON)

set(BUILD_AUV2_EFFECT_PLUGIN ON)
set(BUILD_AUV2_SYNTH_PLUGIN ON)

# Initialize vcpkg. Must be before project() call.
include(cmake/init_vcpkg.cmake)

project(Cabbage VERSION 0.0.98)

enable_language(CXX)
if(APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)
endif()

# Set default build type to Release.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

# Initialize dependencies.
include(cmake/fetch_dependencies.cmake)
include(${iplug2_SOURCE_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

include("${IPLUG2_CMAKE_DIR}/APP.cmake")

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


set(CABBAGE_OPCODE_SOURCES
  "src/opcodes/CabbageGetOpcodes.h"
  "src/opcodes/CabbageGetOpcodes.cpp"
  "src/opcodes/CabbageSetOpcodes.h"
  "src/opcodes/CabbageSetOpcodes.cpp"
  "src/opcodes/CabbageProfilerOpcodes.h"
  "src/opcodes/CabbageProfilerOpcodes.cpp"
  "src/opcodes/CabbageOpcodes.h"
)

# Add Cabbage targets.
if(APPLE)
    # TODO: Remove this if not needed.
    find_package(ZLIB REQUIRED)

    set(CABBAGE_DEFINES
        OBJC_PREFIX=vCabbage
        SWELL_APP_PREFIX=Swell_vCabbage
        SWELL_COMPILED
    )

    set(CABBAGE_INCLUDE_DIRECTORIES
        ${iplug2_SOURCE_DIR}/Dependencies/Build/mac/include
        "src"
        "src/app"
    )

    # Very possible we don't need some of these - not super familiar with mac frameworks
    set(CABBAGE_LINK_LIBRARIES
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

    set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} "/Library/Frameworks/CsoundLib64")

    find_library(CSOUND_LIBRARY
        NAMES CsoundLib64
        HINTS
            "/Library/Frameworks/CsoundLib64.framework"
            "/Library/Frameworks/CsoundLib64.framework/"
    )

    include_directories("/Library/Frameworks/CsoundLib64.framework/Headers")

    set(CABBAGE_WEBVIEW_SOURCES
        "src/webView/IPlugWebView.h"
        "src/webView/IPlugWebView.mm"
        "src/webView/CabbageEditorDelegate.h"
        "src/webView/CabbageEditorDelegate.mm"
    )

    add_library(_base INTERFACE)

    iplug_target_add(_base INTERFACE
        SOURCE ${CABBAGE_WEB_RESOURCES}
        INCLUDE ${iplug2_SOURCE_DIR}/Dependencies/Extras/nlohmann ${CABBAGE_INCLUDE_DIRECTORIES} ${CMAKE_SOURCE_DIR}/resources
        LINK iPlug2_GL2 ${CABBAGE_LINK_LIBRARIES} ${CSOUND_LIBRARY} ${LIB_SNDFILE}
        FEATURE cxx_std_17
        DEFINE NO_IGRAPHICS ${CABBAGE_DEFINES} CUSTOM_EDITOR="${CMAKE_SOURCE_DIR}/src/webView/CabbageEditorDelegate.h" CUSTOM_EDITOR_CLASS=CabbageEditorDelegate
    )

    if(BUILD_STANDALONE_APP)
        add_executable(Cabbage MACOSX_BUNDLE ${CABBAGE_SOURCES} ${CABBAGE_WEBVIEW_SOURCES} ${CABBAGE_OPCODE_SOURCES})

        iplug_target_add(Cabbage PUBLIC
            LINK iPlug2_APP ZLIB::ZLIB _base ${CSOUND_LIBRARY} ${LIB_SNDFILE} RESOURCE ${RESOURCES}
            DEFINE Cabbage
        )

        iplug_configure_target(Cabbage app)

        set_target_properties(Cabbage PROPERTIES XCODE_ATTRIBUTE_PRODUCT_NAME Cabbage)
    endif()

    if(BUILD_VSCODE_SERVICE_APP)
        add_executable(CabbageVSCodeService MACOSX_BUNDLE ${SRC_FILES} ${WEBVIEW_SRC_FILES} ${OPCODE_SRC_FILES})

        iplug_target_add(CabbageVSCodeService PUBLIC
            LINK iPlug2_APP ZLIB::ZLIB _base ${CSOUND_LIBRARY} ${LIB_SNDFILE} RESOURCE ${RESOURCES}
            DEFINE "CabbageVSCodeService"
        )

        iplug_configure_target(CabbageVSCodeService app)
    endif()

    if(BUILD_VST3_EFFECT_PLUGIN)
        add_library(CabbageVST3Effect MODULE ${CABBAGE_SOURCES} ${CABBAGE_WEBVIEW_SOURCES} ${CABBAGE_OPCODE_SOURCES})
        iplug_target_add(CabbageVST3Effect PUBLIC
            LINK iPlug2_VST3 ZLIB::ZLIB _base RESOURCE ${RESOURCES}
            DEFINE "CabbageVST3Effect"
        )

        iplug_configure_target(CabbageVST3Effect vst3)

        set_target_properties(CabbageVST3Effect PROPERTIES XCODE_ATTRIBUTE_PRODUCT_NAME "CabbageVST3Effect")

        add_custom_command(TARGET CabbageVST3Effect POST_BUILD
            COMMAND ${CMAKE_COMMAND} ARGS "-E" "copy_directory" "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/CabbageVST3Effect.vst3" "$ENV{HOME}/Library/Audio/Plug-Ins/VST3"
        )
      endif()

      if(BUILD_AUV2_EFFECT_PLUGIN)
        add_library(CabbageAUV2Effect MODULE ${CABBAGE_SOURCES} ${CABBAGE_WEBVIEW_SOURCES} ${CABBAGE_OPCODE_SOURCES})

        iplug_target_add(CabbageAUV2Effect PUBLIC
            LINK iPlug2_AUv2 ZLIB::ZLIB _base RESOURCE ${RESOURCES}
        )

        iplug_configure_target(CabbageAUV2Effect auv2)

        set_target_properties(CabbageAUV2Effect PROPERTIES XCODE_ATTRIBUTE_PRODUCT_NAME CabbageAUV2Effect)
      endif()
endif()
