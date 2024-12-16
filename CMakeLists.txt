cmake_minimum_required(VERSION 3.26 FATAL_ERROR)

include("cmake/cabbage_version.cmake")

# Define IS_CABBAGE_CI_BUILD=true to enable CI-specific checks.
if(DEFINED IS_CABBAGE_CI_BUILD)
    message(DEBUG "IS_CABBAGE_CI_BUILD = ${IS_CABBAGE_CI_BUILD}")
else()
    set(IS_CABBAGE_CI_BUILD false)
endif()

# Set default CMAKE_BUILD_TYPE to Release.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
message(DEBUG "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

# Set default CMAKE_MESSAGE_LOG_LEVEL to STATUS.
if(NOT DEFINED CMAKE_MESSAGE_LOG_LEVEL)
    set(CMAKE_MESSAGE_LOG_LEVEL STATUS)
endif()

# Make sure Apple builds use the Xcode generetor.
if(APPLE AND NOT "Xcode" STREQUAL "${CMAKE_GENERATOR}")
    message(FATAL_ERROR "Apple builds must use the Xcode CMake generator (e.g. `cmake -B build -G Xcode`)")
endif()

if(NOT DEFINED CABBAGE_BUILD_TARGET)
    project(BuildAllCabbageProjects VERSION ${CABBAGE_VERSION})

    if(APPLE)
        set(CABBAGE_BUILD_TARGETS
            CabbageApp
            CabbageStandaloneApp
            CabbageAUv2Effect
            CabbageVST3Effect
            # CabbageAUv2Synth
            CabbageVST3Synth
        )
    else()
        set(CABBAGE_BUILD_TARGETS
            CabbageApp
            CabbageStandaloneApp
            CabbageVST3Effect
            CabbageVST3Synth
        )
    endif()

    message(DEBUG "CMAKE_MESSAGE_LOG_LEVEL = ${CMAKE_MESSAGE_LOG_LEVEL}")

    foreach(CABBAGE_BUILD_TARGET ${CABBAGE_BUILD_TARGETS})
        execute_process(
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            COMMAND ${CMAKE_COMMAND}
                -B "${CMAKE_BINARY_DIR}/${CABBAGE_BUILD_TARGET}"
                -S "${CMAKE_SOURCE_DIR}"
                -G ${CMAKE_GENERATOR}
                -D IS_CABBAGE_CI_BUILD=${IS_CABBAGE_CI_BUILD}
                -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -D CABBAGE_BUILD_TARGET=${CABBAGE_BUILD_TARGET}
                -D CMAKE_MESSAGE_LOG_LEVEL=${CMAKE_MESSAGE_LOG_LEVEL}
            RESULT_VARIABLE exit_code
        )

        if(exit_code)
            message(FATAL_ERROR "CMake step for ${CABBAGE_BUILD_TARGET} failed with exit code ${exit_code}")
        endif()

        add_custom_target(
            ${CABBAGE_BUILD_TARGET}
            ALL
            COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}/${CABBAGE_BUILD_TARGET}" --config ${CMAKE_BUILD_TYPE}
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/${CABBAGE_BUILD_TARGET}"
        )
    endforeach()
elseif(EXISTS "cmake/${CABBAGE_BUILD_TARGET}.cmake")
    include("cmake/${CABBAGE_BUILD_TARGET}.cmake")
else()
    message(FATAL_ERROR "Unsupported CABBAGE_BUILD_TARGET: ${CABBAGE_BUILD_TARGET}")
endif()


