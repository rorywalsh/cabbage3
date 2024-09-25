cmake_minimum_required(VERSION 3.26 FATAL_ERROR)

include(cmake/init_vcpkg.cmake)

project(Cabbage VERSION 0.0.98 LANGUAGES CXX)

# Set default build type to Release if not specified.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

include(cmake/fetch_dependencies.cmake)
