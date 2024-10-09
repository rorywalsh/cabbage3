cmake_minimum_required(VERSION 3.26 FATAL_ERROR) # What CMAKE version am I even *on* on this thing
cmake_policy(SET CMP0091 NEW)

#########

project(Cabbage LANGUAGES C CXX)

if(DEFINED CabbagePluginEffect)
  message("Defining CabbagePluginEffect")
  set(ProductName "CabbagePluginEffect")
elseif(DEFINED CabbageApp)
  message("Defining CabbageApp")
  set(ProductName "CabbageApp")
elseif(DEFINED CabbagePluginSynth)
  message("Defining CabbagePluginSynth")
  set(ProductName "CabbagePluginSynth")
endif()

if(NOT DEFINED BUILD_VERSION)
	set(BUILD_VERSION 0.0.109)
endif()


if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release) # Set default build type to Release if not specified
endif()

set(IPLUG2_DIR ${CMAKE_SOURCE_DIR}/../iPlug2)
set(CHOC_DIR ${CMAKE_SOURCE_DIR}/../choc-main)
set(IXWebSocket_DIR ${CMAKE_SOURCE_DIR}/../IXWebSocket)
include(${IPLUG2_DIR}/iPlug2.cmake)

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)




find_package(iPlug2)

include(FetchContent)

FetchContent_Declare(
  readerwriterqueue
  GIT_REPOSITORY    https://github.com/cameron314/readerwriterqueue
  GIT_TAG           master
)

FetchContent_MakeAvailable(readerwriterqueue)

set(PROJECT_DIR "${CMAKE_SOURCE_DIR}")


#==========================================================================
if(WIN32) 
  set(VCPKG_CMAKE "C:/Users/walshr/sourcecode/vcpkg/scripts/buildsystems/vcpkg.cmake")
  SET(CMAKE_TOOLCHAIN_FILE ${VCPKG_CMAKE})
  set(ZLIB_LIBRARY "C:\\Users\\rory\\sourcecode\\vcpkg\\packages\\zlib_x64-windows-static\\lib\\zlib.lib")
  set(ZLIB_INCLUDE_DIR "C:\\Users\\rory\\sourcecode\\vcpkg\\packages\\zlib_x64-windows-static\\include")

  # # Courtesy of rovingeye, again
  execute_process(
    # requires NuGet
    COMMAND nuget restore -PackagesDirectory "${PROJECT_DIR}/packages"
    WORKING_DIRECTORY ${PROJECT_DIR}
  )

  cmake_path( 
    SET WEBVIEW2_SOURCE_DIR 
    "${PROJECT_DIR}/packages/Microsoft.Web.WebView2.1.0.2478.35"
  )

  cmake_path( 
    SET WIL_SOURCE_DIR 
    "${PROJECT_DIR}/packages/Microsoft.Windows.ImplementationLibrary.1.0.240122.1"
  )
  set(EXTRA_INCLUDE_DIRECTORIES 
    ${WEBVIEW2_SOURCE_DIR}/build/native/include 
    ${WIL_SOURCE_DIR}/include
    ${CHOC_DIR}
    ${IXWebSocket_DIR}

    ${PROJECT_DIR}/src/
    ${PROJECT_DIR}/src/app
  )
  set(EXTRA_LINK_DIRECTORIES 
    ${WEBVIEW2_SOURCE_DIR}/build/native/x64
    ${IXWebSocket_DIR}/build
  )
  set(EXTRA_LINK_LIBRARIES 
    WebView2LoaderStatic.lib 
    readerwriterqueue
    "${IXWebSocket_DIR}/build/Debug/ixwebsocket.lib"
  )

  # set(IWEBVIEW_SOURCE 
  #   ${IPLUG2_DIR}/IPlug/Extras/WebView/IPlugWebView.cpp 
  #   ${IPLUG2_DIR}/IPlug/Extras/WebView/IPlugWebViewEditorDelegate.cpp
  # )


  find_package(ZLIB REQUIRED) 
  include_directories("C:\\Program Files\\Csound6_x64\\include\\csound" "C:\\SDKs\\ASIOSDK2.3.3\\asiosdk_2.3.3_2019-06-14\\common")
  find_library(CSOUND_LIBRARY NAMES csound64 HINTS "c:\\Program Files\\Csound6_x64\\lib")

  set(WEBVIEW_SRC_FILES
  ${IPLUG2_DIR}/IPlug/IPlugPaths.mm
  ${PROJECT_DIR}/src/webView/IPlugWebView.cpp 
  ${PROJECT_DIR}/src/webView/CabbageEditorDelegate.cpp
  ${PROJECT_DIR}/src/webView/IPlugWebView.h 
  ${PROJECT_DIR}/src/webView/CabbageEditorDelegate.h
  )

# ================================================================================
elseif(APPLE) 
  # thanks ben vining!!
  # On further inspection, this is done in the top-level cmake file anyway - but maybe doesn't hurt for OOS builds and verbosity
  enable_language(OBJC)
  enable_language(OBJCXX)
  find_package(ZLIB REQUIRED)
  set(EXTRA_DEFINES 
    OBJC_PREFIX=vCabbage
    SWELL_APP_PREFIX=Swell_vCabbage
    SWELL_COMPILED
  )

  set(EXTRA_INCLUDE_DIRECTORIES 
    ${IPLUG2_DIR}/Dependencies/Build/mac/include
    ${IXWebSocket_DIR}
    ${PROJECT_DIR}/src/
    ${PROJECT_DIR}/src/app
  )

  # find_package(OpenSSL REQUIRED)

  set(EXTRA_LINK_DIRECTORIES 
    ${IXWebSocket_DIR}/build
  )

  # Very possible we don't need some of these - not super familiar with mac frameworks
  set(EXTRA_LINK_LIBRARIES 
    "-framework Carbon -framework AppKit -framework Metal -framework MetalKit -framework WebKit -framework CoreFoundation"
    "${IXWebSocket_DIR}/build/libixwebsocket.a"
    readerwriterqueue
  )
  
  # because the file hierarchy needs preserving, I did this the KISS way - but maybe some of the existing iPlug stuff has a more elegant way to do this
  set(WEB_RESOURCES 
    ${PROJECT_DIR}/resources/web/index.html
    ${PROJECT_DIR}/resources/web/script.js
    ${PROJECT_DIR}/resources/web/webaudio-controls.js
  )

  set_source_files_properties(${PROJECT_DIR}/resources/web/index.html PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/web")
  set_source_files_properties(${PROJECT_DIR}/resources/web/script.js PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/web")
  set_source_files_properties(${PROJECT_DIR}/resources/web/webaudio-controls.js PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/web")

  SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} "/Applications/Csound/CsoundLib64.framework")
  find_library(CSOUND_LIBRARY NAMES CsoundLib64 HINTS /Applications/Csound/CsoundLib64.framework/
  "/Applications/Csound/CsoundLib64.framework/")  
  include_directories("/Applications/Csound/CsoundLib64.framework/Headers")

  set(WEBVIEW_SRC_FILES
  ${PROJECT_DIR}/src/webView/IPlugWebView.mm
  ${PROJECT_DIR}/src/webView/CabbageEditorDelegate.mm
  ${PROJECT_DIR}/src/webView/IPlugWebView.h 
  ${PROJECT_DIR}/src/webView/CabbageEditorDelegate.h
  )

endif()

set(SRC_FILES
  ${PROJECT_DIR}/src/config.h
  ${PROJECT_DIR}/src/CabbageProcessor.h
  ${PROJECT_DIR}/src/CabbageProcessor.cpp
  ${PROJECT_DIR}/src/CabbageParser.h
  ${PROJECT_DIR}/src/Cabbage.h
  ${PROJECT_DIR}/src/Cabbage.cpp
  ${PROJECT_DIR}/src/CabbageUtils.h
  ${PROJECT_DIR}/src/CabbageServer.cpp
  ${PROJECT_DIR}/src/CabbageServer.h
)


set(OPCODE_SRC_FILES
  ${PROJECT_DIR}/src/opcodes/CabbageGetOpcodes.h
  ${PROJECT_DIR}/src/opcodes/CabbageGetOpcodes.cpp
  ${PROJECT_DIR}/src/opcodes/CabbageSetOpcodes.h
  ${PROJECT_DIR}/src/opcodes/CabbageSetOpcodes.cpp
  ${PROJECT_DIR}/src/opcodes/CabbageProfilerOpcodes.h
  ${PROJECT_DIR}/src/opcodes/CabbageProfilerOpcodes.cpp
  ${PROJECT_DIR}/src/opcodes/CabbageOpcodes.h
)

#find_package(iPlug2 REQUIRED COMPONENTS Skia)


# For typing convenience the TARGET name is put into a variable.
if(WIN32) 
  # While not required, creating a base interface for includes and settings seems like a good idea.
  add_library(_base INTERFACE)
  # iplug_target_add() is a shorthand function for adding sources and include directories,
  # linking libraries, adding resources, setting compile options, etc.
  iplug_target_add(_base INTERFACE  
    SOURCE ${WEB_RESOURCES}
    INCLUDE ${PROJECT_DIR} ${IPLUG2_DIR}/Dependencies/Extras/nlohmann ${EXTRA_INCLUDE_DIRECTORIES} ${PROJECT_DIR}/resources
    LINK_DIR ${EXTRA_LINK_DIRECTORIES}
    LINK iPlug2_GL2 ${EXTRA_LINK_LIBRARIES} ${CSOUND_LIBRARY} ${LIB_SNDFILE}
    FEATURE cxx_std_17
    DEFINE NO_IGRAPHICS ${EXTRA_DEFINES} CUSTOM_EDITOR="${PROJECT_DIR}/src/webView/CabbageEditorDelegate.h" CUSTOM_EDITOR_CLASS=CabbageEditorDelegate)

  if (DEFINED CabbageApp)
    set(CUSTOM_APP_SRC
    ../src/app/CabbageAPP.cpp 
    ../src/app/CabbageAPP_host.h
    ../src/app/CabbageAPP_dialog.cpp 
    ../src/app/CabbageAPP_host.cpp 
    ../src/app/CabbageAPP_main.cpp
    ../src/app/CabbageAPP.h
    )
    # this is the taget for the command line app that runs from vscode
    set(TARGET app)    
    add_executable(${TARGET} WIN32 ${SRC_FILES} ${WEBVIEW_SRC_FILES} ${OPCODE_SRC_FILES})
    iplug_target_add(${TARGET} PUBLIC 
    LINK iPlug2_APP ZLIB::ZLIB _base  ${CSOUND_LIBRARY} ${LIB_SNDFILE} RESOURCE ${RESOURCES} 
    DEFINE ${ProductName} WIN32_LEAN_AND_MEAN CUSTOM_APP_SRC)
    iplug_configure_target(${TARGET} app)
    source_group("Cabbage App" FILES ${CUSTOM_APP_SRC})
  else()

    message("Setting up standalone app taget")
    

    set(TARGET app)
    add_executable(${TARGET}  WIN32 ${SRC_FILES} ${WEBVIEW_SRC_FILES} ${OPCODE_SRC_FILES})
    iplug_target_add(${TARGET} PUBLIC 
    LINK iPlug2_APP ZLIB::ZLIB _base  ${CSOUND_LIBRARY} ${LIB_SNDFILE} RESOURCE ${RESOURCES} 
    DEFINE WIN32_LEAN_AND_MEAN)
    iplug_configure_target(${TARGET} app )

    message("Setting up VST3 target")
    set(TARGET vst3)
    add_library(${TARGET} MODULE ${SRC_FILES} ${WEBVIEW_SRC_FILES} ${OPCODE_SRC_FILES})
    iplug_target_add(${TARGET} PUBLIC 
    LINK iPlug2_VST3 ZLIB::ZLIB _base RESOURCE ${RESOURCES}
    DEFINE WIN32_LEAN_AND_MEAN)
    iplug_configure_target(${TARGET} vst3)

    set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME ${ProductName})
  endif()
# add_executable(${TARGET} WIN32 MACOSX_BUNDLE  ${WEBVIEW_SRC_FILES} ${SRC_FILES} ${OPCODE_SRC_FILES} )
# iplug_target_add(${TARGET} PUBLIC 
# LINK iPlug2_APP _base  ${CSOUND_LIBRARY} ${LIB_SNDFILE} RESOURCE ${RESOURCES} DEFINE WIN32_LEAN_AND_MEAN CUSTOM_APP_SRC)
# iplug_configure_target(${TARGET} app)
else()
  set_source_files_properties(${PROJECT_DIR}/src/webView/IPlugWebView.mm PROPERTIES COMPILE_OPTIONS -fobjc-arc)

  # While not required, creating a base interface for includes and settings seems like a good idea.
  add_library(_base INTERFACE)
  # iplug_target_add() is a shorthand function for adding sources and include directories,
  # linking libraries, adding resources, setting compile options, etc.
  iplug_target_add(_base INTERFACE  
    SOURCE ${WEB_RESOURCES}
    INCLUDE ${PROJECT_DIR} ${IPLUG2_DIR}/Dependencies/Extras/nlohmann ${EXTRA_INCLUDE_DIRECTORIES} ${PROJECT_DIR}/resources
    LINK_DIR ${EXTRA_LINK_DIRECTORIES}
    LINK iPlug2_GL2 ${EXTRA_LINK_LIBRARIES} ${CSOUND_LIBRARY} ${LIB_SNDFILE}
    FEATURE cxx_std_17
    DEFINE NO_IGRAPHICS ${EXTRA_DEFINES} CUSTOM_EDITOR="${PROJECT_DIR}/src/webView/CabbageEditorDelegate.h" CUSTOM_EDITOR_CLASS=CabbageEditorDelegate
  )

  find_library(CsoundLibrary NAMES CsoundLib64 PATHS "/Applications/Csound" REQUIRED NO_DEFAULT_PATH)

    if (DEFINED CabbageApp)
      set(CUSTOM_APP_SRC
      ../src/app/CabbageAPP.cpp 
      ../src/app/CabbageAPP_host.h
      ../src/app/CabbageAPP_dialog.cpp 
      ../src/app/CabbageAPP_host.cpp 
      ../src/app/CabbageAPP_main.cpp
      ../src/app/CabbageAPP.h
      )
      # this is the taget for the command line app that runs from vscode
      set(TARGET app)    
      add_executable(${TARGET} MACOSX_BUNDLE ${SRC_FILES} ${WEBVIEW_SRC_FILES} ${OPCODE_SRC_FILES})
      iplug_target_add(${TARGET} PUBLIC 
      LINK iPlug2_APP ZLIB::ZLIB _base ${CsoundLibrary}  ${LIB_SNDFILE} RESOURCE ${RESOURCES} 
      DEFINE ${ProductName})
      iplug_configure_target(${TARGET} app)
      set_target_properties(${TARGET} PROPERTIES XCODE_ATTRIBUTE_PRODUCT_NAME ${ProductName})
      source_group("Cabbage App" FILES ${CUSTOM_APP_SRC})
    else()
      # this app is the stadnalone target export from Cabbage
      set(TARGET app)
      
      add_executable(${TARGET} MACOSX_BUNDLE ${SRC_FILES} ${WEBVIEW_SRC_FILES} ${OPCODE_SRC_FILES})
      iplug_target_add(${TARGET} PUBLIC 
      LINK iPlug2_APP ZLIB::ZLIB _base ${CsoundLibrary} ${LIB_SNDFILE} RESOURCE ${RESOURCES} 
      DEFINE ${ProductName})
      iplug_configure_target(${TARGET} app )
      set_target_properties(${TARGET} PROPERTIES XCODE_ATTRIBUTE_PRODUCT_NAME ${ProductName})

      

      set(TARGET vst3)
      add_library(${TARGET} MODULE ${SRC_FILES} ${WEBVIEW_SRC_FILES} ${OPCODE_SRC_FILES})
      iplug_target_add(${TARGET} PUBLIC 
      LINK iPlug2_VST3 ZLIB::ZLIB _base RESOURCE ${RESOURCES}
      DEFINE ${ProductName})
      iplug_configure_target(${TARGET} vst3)
      set_target_properties(${TARGET} PROPERTIES XCODE_ATTRIBUTE_PRODUCT_NAME ${ProductName})
      add_custom_command(TARGET ${TARGET} POST_BUILD
      COMMAND ${CMAKE_COMMAND} ARGS "-E" "copy_directory" "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${ProductName}.vst3" "$ENV{HOME}/Library/Audio/Plug-Ins/VST3")

      # set(TARGET vst2)
      # add_library(${TARGET} MODULE ${SRC_FILES} ${WEBVIEW_SRC_FILES} ${OPCODE_SRC_FILES})
      # iplug_target_add(${TARGET} PUBLIC 
      # LINK iPlug2_VST2 ZLIB::ZLIB _base RESOURCE ${RESOURCES}
      # DEFINE ${ProductName})
      # iplug_configure_target(${TARGET} vst2)
      # set_target_properties(${TARGET} PROPERTIES XCODE_ATTRIBUTE_PRODUCT_NAME ${ProductName})
      # add_custom_command(TARGET ${TARGET} POST_BUILD
      # COMMAND ${CMAKE_COMMAND} ARGS "-E" "copy_directory" "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${ProductName}.vst3" "$ENV{HOME}/Library/Audio/Plug-Ins/VST3")


      if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
        set(TARGET auv2)
        add_library(${TARGET} MODULE ${SRC_FILES} ${WEBVIEW_SRC_FILES} ${OPCODE_SRC_FILES})
        iplug_target_add(${TARGET} PUBLIC 
        LINK iPlug2_AUv2 ZLIB::ZLIB _base RESOURCE ${RESOURCES})
        iplug_configure_target(${TARGET} auv2)
        set_target_properties(${TARGET} PROPERTIES XCODE_ATTRIBUTE_PRODUCT_NAME ${ProductName})
      endif()
    endif()
endif()

source_group("Cabbage Base" FILES ${SRC_FILES} )
source_group("Webview" FILES ${WEBVIEW_SRC_FILES})
source_group("Opcodes" FILES ${OPCODE_SRC_FILES})

if (WIN32)
  set(CMAKE_RC_FLAGS "/I${PROJECT_DIR}/resources /I${PROJECT_DIR}/resources/fonts /I${PROJECT_DIR}/resources/img ${CMAKE_RC_FLAGS}")
endif()
