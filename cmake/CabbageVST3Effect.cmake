set(CABBAGE_PROJECT_NAME ${CABBAGE_BUILD_TARGET})

include("cmake/cabbage_project.cmake")

add_library(${CABBAGE_PROJECT_NAME} MODULE
    ${CABBAGE_OPCODE_SOURCES}
    ${CABBAGE_SOURCES}
    ${CABBAGE_WEBVIEW_SOURCES}
)

iplug_target_add(${CABBAGE_PROJECT_NAME} PUBLIC
    DEFINE
        CabbagePluginEffect
    INCLUDE
        "${CMAKE_SOURCE_DIR}/resources"
    LINK
        _base
        iPlug2_VST3
    RESOURCE ${RESOURCES}
)

iplug_configure_target(${CABBAGE_PROJECT_NAME} vst3)

set_target_properties(${CABBAGE_PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_PRODUCT_NAME "${CABBAGE_PROJECT_NAME}")

# TODO: Make a target that depends on the build .vst3 file.
add_custom_command(TARGET ${CABBAGE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} ARGS "-E" "copy_directory" "${CMAKE_BINARY_DIR}/out/${CABBAGE_PROJECT_NAME}.vst3" "$ENV{HOME}/Library/Audio/Plug-Ins/VST3"
)
