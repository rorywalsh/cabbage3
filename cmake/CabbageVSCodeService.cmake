set(CABBAGE_PROJECT_NAME ${CABBAGE_BUILD_TARGET})

include("cmake/cabbage_project.cmake")

add_executable(${CABBAGE_PROJECT_NAME} MACOSX_BUNDLE
    ${CABBAGE_SOURCES}
    ${CABBAGE_OPCODE_SOURCES}
    ${CABBAGE_WEBVIEW_SOURCES}
)

iplug_target_add(${CABBAGE_PROJECT_NAME} PUBLIC
    INCLUDE
        "${CMAKE_SOURCE_DIR}/resources"
    LINK
        _base
        iPlug2_APP
        ${CSOUND_FRAMEWORK}
    RESOURCE ${RESOURCES}
)

iplug_configure_target(${CABBAGE_PROJECT_NAME} app)

set_target_properties(${CABBAGE_PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_PRODUCT_NAME "${CABBAGE_PROJECT_NAME}")
