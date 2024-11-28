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
        iPlug2_AUv2
    RESOURCE ${RESOURCES}
)

iplug_configure_target(${CABBAGE_PROJECT_NAME} auv2)

set_target_properties(${CABBAGE_PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_PRODUCT_NAME "${CABBAGE_PROJECT_NAME}")
