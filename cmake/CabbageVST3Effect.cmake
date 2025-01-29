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

set(SMTG_OS_LINUX "ON")

target_compile_options(${CABBAGE_PROJECT_NAME} PRIVATE -Wno-error)
iplug_configure_target(${CABBAGE_PROJECT_NAME} vst3)

set_target_properties(${CABBAGE_PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_PRODUCT_NAME "${CABBAGE_PROJECT_NAME}")

if(APPLE)
# TODO: Make a target that depends on the build .vst3 file.
add_custom_command(TARGET ${CABBAGE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} ARGS "-E" "copy_directory" "${CMAKE_BINARY_DIR}/out/${CABBAGE_PROJECT_NAME}.vst3" "$ENV{HOME}/Library/Audio/Plug-Ins/VST3"
)
elseif (LINUX)
    # The target directory where the plugin host expects the .vst3 directory
    # Define the paths
    set(SYMLINK_PATH "$ENV{HOME}/.vst3/CabbageVST3Effect.vst3")
    set(TARGET_OUTPUT_PATH "${CMAKE_HOME_DIRECTORY}/cmake-build-debug/CabbageVST3Effect/out/CabbageVST3Effect.vst3")

    add_custom_command(
            TARGET ${CABBAGE_PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory $ENV{HOME}/.vst3
            # Remove the existing directory or symlink, if any
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${SYMLINK_PATH}
            # Create the symbolic link
            COMMAND ${CMAKE_COMMAND} -E create_symlink ${TARGET_OUTPUT_PATH} ${SYMLINK_PATH}
            COMMENT "Creating symbolic link in ~/.vst3 for plugin host"
    )

endif()
