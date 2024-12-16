include_guard()

# Comment out the following line to see each package's CMake usage instructions.
set(VCPKG_INSTALL_OPTIONS "--no-print-usage")

# Include this file before calling `project(...)` in CMakeLists.txt, so the vcpkg toolchain gets used.

message(STATUS "Fetching vcpkg")

include(FetchContent)

FetchContent_Declare(
    vcpkg
    URL https://github.com/microsoft/vcpkg/archive/refs/tags/2024.08.23.tar.gz
    URL_MD5 38e70bc31a543500f541427448b95f2a
    DOWNLOAD_DIR "${FETCHCONTENT_BASE_DIR}/vcpkg"
    DOWNLOAD_EXTRACT_TIMESTAMP true
)

FetchContent_MakeAvailable(vcpkg)

message(STATUS "Fetching vcpkg - done")

set(VCPKG_INSTALLED_DIR "${FETCHCONTENT_BASE_DIR}/vcpkg-installed")
set(CMAKE_TOOLCHAIN_FILE "${vcpkg_SOURCE_DIR}/scripts/buildsystems/vcpkg.cmake")
set(X_VCPKG_APPLOCAL_DEPS_INSTALL ON CACHE BOOL "" FORCE)

if(WIN32)
    set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "Triplet for vcpkg")
endif()