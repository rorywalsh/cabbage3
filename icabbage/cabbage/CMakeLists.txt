cmake_minimum_required(VERSION 3.26 FATAL_ERROR) # What CMAKE version am I even *on* on this thing
cmake_policy(SET CMP0091 NEW)

#########



project(Cabbage VERSION 1.0.0 LANGUAGES C CXX)

if(NOT DEFINED BUILD_VERSION)
	set(BUILD_VERSION 0.0.7)
endif()

set(IPLUG2_DIR ${CMAKE_SOURCE_DIR}/../iPlug2)
set(IXWebSocket_DIR ${CMAKE_SOURCE_DIR}/../IXWebSocket)
include(${IPLUG2_DIR}/iPlug2.cmake)

find_package(iPlug2)

set(PROJECT_DIR "${CMAKE_SOURCE_DIR}")

#==========================================================================
if(WIN32) 
  set(VCPKG_CMAKE "C:/Users/walshr/sourcecode/vcpkg/scripts/buildsystems/vcpkg.cmake")
  SET(CMAKE_TOOLCHAIN_FILE ${VCPKG_CMAKE})
  set(ZLIB_LIBRARY "C:\\Users\\walshr\\sourcecode\\vcpkg\\packages\\zlib_x64-windows-static\\lib\\zlib.lib")
  set(ZLIB_INCLUDE_DIR "C:\\Users\\walshr\\sourcecode\\vcpkg\\packages\\zlib_x64-windows-static\\include")
  
  # Courtesy of rovingeye, again
  execute_process(
    # requires NuGet
    COMMAND nuget restore -PackagesDirectory "${CMAKE_BINARY_DIR}/_deps/Nuget"
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )

  cmake_path( 
    SET WEBVIEW2_SOURCE_DIR 
    "${CMAKE_BINARY_DIR}/_deps/Nuget/Microsoft.Web.WebView2.1.0.1587.40"
  )

  cmake_path( 
    SET WIL_SOURCE_DIR 
    "${CMAKE_BINARY_DIR}/_deps/Nuget/Microsoft.Windows.ImplementationLibrary.1.0.230202.1"
  )
  set(EXTRA_INCLUDE_DIRECTORIES 
    ${WEBVIEW2_SOURCE_DIR}/build/native/include 
    ${WIL_SOURCE_DIR}/include
  )
  set(EXTRA_LINK_DIRECTORIES 
    ${WEBVIEW2_SOURCE_DIR}/build/native/x64
  )
  set(EXTRA_LINK_LIBRARIES 
    WebView2LoaderStatic.lib 

  )

  set(IWEBVIEW_SOURCE 
    ${IPLUG2_DIR}/IPlug/Extras/WebView/IPlugWebView.cpp 
    ${IPLUG2_DIR}/IPlug/Extras/WebView/IPlugWebViewEditorDelegate.cpp
  )


  find_package(ZLIB REQUIRED) 
  include_directories("C:\\Program Files\\Csound6_x64\\include\\csound" "C:\\SDKs\\ASIOSDK2.3.3\\asiosdk_2.3.3_2019-06-14\\common")
  find_library(CSOUND_LIBRARY NAMES csound64 HINTS "c:\\Program Files\\Csound6_x64\\lib")
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
  )

  # find_package(OpenSSL REQUIRED)

  set(EXTRA_LINK_DIRECTORIES 
    ${IXWebSocket_DIR}/build
  )

  # Very possible we don't need some of these - not super familiar with mac frameworks
  set(EXTRA_LINK_LIBRARIES 
    "-framework Carbon -framework AppKit -framework Metal -framework MetalKit -framework WebKit -framework CoreFoundation"
    "${IXWebSocket_DIR}/build/libixwebsocket.a"
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

  SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} "/Library/Frameworks/CsoundLib64")
  find_library(CSOUND_LIBRARY NAMES CsoundLib64 HINTS /Library/Frameworks/CsoundLib64.framework/
  "/Library/Frameworks/CsoundLib64.framework")  
  include_directories("/Library/Frameworks/CsoundLib64.framework/Headers")

endif()

#find_package(iPlug2 REQUIRED COMPONENTS Skia)
set(SRC_FILES
  ${PROJECT_DIR}/config.h
  ${PROJECT_DIR}/CabbageProcessor.h
  ${PROJECT_DIR}/CabbageProcessor.cpp
  ${PROJECT_DIR}/CabbageParser.h
  ${PROJECT_DIR}/Cabbage.h
  ${PROJECT_DIR}/Cabbage.cpp
  ${PROJECT_DIR}/CabbageWidgetDescriptors.h
  ${PROJECT_DIR}/WebView/IPlugWebView.mm 
  ${PROJECT_DIR}/WebView/IPlugWebViewEditorDelegate.mm
  ${PROJECT_DIR}/WebView/IPlugWebView.h 
  ${PROJECT_DIR}/WebView/IPlugWebViewEditorDelegate.h
)


set_source_files_properties(${PROJECT_DIR}/WebView/IPlugWebView.mm PROPERTIES COMPILE_OPTIONS -fobjc-arc)

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
  DEFINE NO_IGRAPHICS ${EXTRA_DEFINES} CUSTOM_EDITOR="${PROJECT_DIR}/WebView/IPlugWebViewEditorDelegate.h" CUSTOM_EDITOR_CLASS=WebViewEditorDelegate
)


# For typing convenience the TARGET name is put into a variable.
set(TARGET app)
add_executable(${TARGET} WIN32 MACOSX_BUNDLE ${SRC_FILES} )
iplug_target_add(${TARGET} PUBLIC 
LINK iPlug2_APP ZLIB::ZLIB _base  ${CSOUND_LIBRARY} ${LIB_SNDFILE} RESOURCE ${RESOURCES} DEFINE CUSTOM_APP_SRC)
iplug_configure_target(${TARGET} app)







if (WIN32)
  set(CMAKE_RC_FLAGS "/I${PROJECT_DIR}/resources /I${PROJECT_DIR}/resources/fonts /I${PROJECT_DIR}/resources/img ${CMAKE_RC_FLAGS}")
endif()
