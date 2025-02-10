cmake_minimum_required(VERSION 3.14)
project(WebViewProcess)

# Define FetchContent base directory
set(FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/../../../../cmake-build-debug/_deps")

if(EXISTS "${FETCHCONTENT_BASE_DIR}")
    message(STATUS "FETCHCONTENT_BASE_DIR exists: ${FETCHCONTENT_BASE_DIR}")
else()
    message(WARNING "FETCHCONTENT_BASE_DIR does NOT exist: ${FETCHCONTENT_BASE_DIR}")
endif()



# Define iPlug2 source directory (you should set this to the correct path)
set(iPlug2_SOURCE_DIR "path/to/iPlug2")

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK REQUIRED gtk+-3.0 webkit2gtk-4.1)

# Include directories
include_directories(
    ${FETCHCONTENT_BASE_DIR}/readerwriterqueue-src
    ${iPlug2_SOURCE_DIR}/Dependencies/Extras/nlohmann
    ${GTK_INCLUDE_DIRS}
)

# Define the executable
add_executable(webview webviewProcess.cpp)

# Link libraries
target_link_libraries(webview
    ${GTK_LIBRARIES}
    X11
    Xext
)

# Compiler flags
target_compile_options(webview PRIVATE ${GTK_CFLAGS_OTHER})