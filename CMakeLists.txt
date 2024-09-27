cmake_minimum_required(VERSION 3.26 FATAL_ERROR)

# Define IS_CABBAGE_CI_BUILD=true to enable CI-specific checks.
if(DEFINED IS_CABBAGE_CI_BUILD)
    message(DEBUG "IS_CABBAGE_CI_BUILD = ${IS_CABBAGE_CI_BUILD}")
else()
    set(IS_CABBAGE_CI_BUILD false)
endif()

# Default CMAKE_BUILD_TYPE to Release.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
message(DEBUG "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

if(NOT DEFINED CABBAGE_BUILD_TARGET)
    message(DEBUG "CABBAGE_BUILD_TARGET is not defined. Configuring all Cabbage targets.")

    project(Cabbage VERSION 0.0.100)

    set(CABBAGE_BUILD_TARGETS
        CabbageStandaloneApp
        CabbageVSCodeServiceApp
        CabbageAUv2Effect
        CabbageAUv2Synth
        CabbageVST3Effect
        CabbageVST3Synth
    )

    message(DEBUG "CMAKE_MESSAGE_LOG_LEVEL = ${CMAKE_MESSAGE_LOG_LEVEL}")

    foreach(CABBAGE_BUILD_TARGET ${CABBAGE_BUILD_TARGETS})
        execute_process(
            COMMAND ${CMAKE_COMMAND}
            -B "${CMAKE_BINARY_DIR}/${CABBAGE_BUILD_TARGET}"
            -G ${CMAKE_GENERATOR}
            -D IS_CABBAGE_CI_BUILD=${IS_CABBAGE_CI_BUILD}
            -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -D CABBAGE_BUILD_TARGET=${CABBAGE_BUILD_TARGET}
            -D CABBAGE_VERSION=${CMAKE_PROJECT_VERSION}
            -D CMAKE_MESSAGE_LOG_LEVEL=${CMAKE_MESSAGE_LOG_LEVEL}
        )

        add_custom_target(
            build_${CABBAGE_BUILD_TARGET}
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

if(APPLE)
    # if(BUILD_STANDALONE_APP)
    #     add_executable(Cabbage MACOSX_BUNDLE
    #         ${CABBAGE_OPCODE_SOURCES}
    #         ${CABBAGE_SOURCES}
    #         ${CABBAGE_WEBVIEW_SOURCES}
    #     )

    #     set(PLUG_NAME "Cabbage" CACHE STRING "" FORCE)

    #     iplug_target_add(Cabbage PUBLIC
    #         INCLUDE
    #             "${CMAKE_SOURCE_DIR}/resources"
    #         LINK
    #             _base
    #             iPlug2_APP
    #             ${CSOUND_FRAMEWORK}
    #         RESOURCE ${RESOURCES}
    #     )

    #     iplug_configure_target(Cabbage app)

    #     set_target_properties(Cabbage PROPERTIES XCODE_ATTRIBUTE_PRODUCT_NAME "Cabbage")
    #     target_link_options(Cabbage PRIVATE LINKER:-adhoc_codesign)
    # endif()

    # if(BUILD_VSCODE_SERVICE_APP)
    #     add_executable(CabbageVSCodeService MACOSX_BUNDLE
    #         ${CABBAGE_SOURCES}
    #         ${CABBAGE_OPCODE_SOURCES}
    #         ${CABBAGE_WEBVIEW_SOURCES}
    #     )

    #     set(PLUG_NAME "CabbageVSCodeService" CACHE STRING "" FORCE)

    #     iplug_target_add(CabbageVSCodeService PUBLIC
    #         INCLUDE
    #             "${CMAKE_SOURCE_DIR}/resources"
    #         LINK
    #             _base
    #             iPlug2_APP
    #             ${CSOUND_FRAMEWORK}
    #         RESOURCE ${RESOURCES}
    #     )

    #     iplug_configure_target(CabbageVSCodeService app)

    #     set_target_properties(CabbageVSCodeService PROPERTIES XCODE_ATTRIBUTE_PRODUCT_NAME "CabbageVSCodeService")
    # endif()

    # if(BUILD_AUV2_EFFECT_PLUGIN)
    #     add_library(CabbageAUV2Effect MODULE
    #         ${CABBAGE_OPCODE_SOURCES}
    #         ${CABBAGE_SOURCES}
    #         ${CABBAGE_WEBVIEW_SOURCES}
    #     )

    #     set(PLUG_NAME "CabbageAUV2Effect" CACHE STRING "" FORCE)

    #     iplug_target_add(CabbageAUV2Effect PUBLIC
    #         INCLUDE
    #             "${CMAKE_SOURCE_DIR}/resources"
    #         LINK
    #             _base
    #             iPlug2_AUv2
    #         RESOURCE ${RESOURCES}
    #     )

    #     iplug_configure_target(CabbageAUV2Effect auv2)

    #     set_target_properties(CabbageAUV2Effect PROPERTIES XCODE_ATTRIBUTE_PRODUCT_NAME "CabbageAUV2Effect")
    # endif()

    # if(BUILD_VST3_EFFECT_PLUGIN)
    #     add_library(CabbageVST3Effect MODULE
    #         ${CABBAGE_OPCODE_SOURCES}
    #         ${CABBAGE_SOURCES}
    #         ${CABBAGE_WEBVIEW_SOURCES}
    #     )

    #     set(PLUG_NAME "CabbageVST3Effect" CACHE STRING "" FORCE)

    #     iplug_target_add(CabbageVST3Effect PUBLIC
    #         INCLUDE
    #             "${CMAKE_SOURCE_DIR}/resources"
    #         LINK
    #             _base
    #             iPlug2_VST3
    #         RESOURCE ${RESOURCES}
    #     )

    #     iplug_configure_target(CabbageVST3Effect vst3)

    #     set_target_properties(CabbageVST3Effect PROPERTIES XCODE_ATTRIBUTE_PRODUCT_NAME "CabbageVST3Effect")

    #     # TODO: Make a target that depends on the build .vst3 file.
    #     add_custom_command(TARGET CabbageVST3Effect POST_BUILD
    #         COMMAND ${CMAKE_COMMAND} ARGS "-E" "copy_directory" "${CMAKE_BINARY_DIR}/out/CabbageVST3Effect.vst3" "$ENV{HOME}/Library/Audio/Plug-Ins/VST3"
    #     )
    # endif()
endif()
