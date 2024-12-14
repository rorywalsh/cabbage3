set(CABBAGE_PROJECT_NAME ${CABBAGE_BUILD_TARGET})

include("cmake/cabbage_project.cmake")



add_executable(${CABBAGE_PROJECT_NAME} MACOSX_BUNDLE
    ${CABBAGE_OPCODE_SOURCES}
    ${CABBAGE_SOURCES}
    ${CABBAGE_WEBVIEW_SOURCES}
)



iplug_target_add(${CABBAGE_PROJECT_NAME} PUBLIC
    DEFINE
        CabbageApp 
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

if(MSVC)
    add_custom_command(
        TARGET ${CABBAGE_PROJECT_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Modifying ${CABBAGE_PROJECT_NAME}.vcxproj file"
        COMMAND powershell -NoProfile -ExecutionPolicy Bypass -Command "
            try {
                (Get-Content '${CMAKE_BINARY_DIR}/${CABBAGE_PROJECT_NAME}.vcxproj') -replace '<RuntimeLibrary>MultiThreadedDebug</RuntimeLibrary>', '<RuntimeLibrary>MultiThreadedDebugDLL</RuntimeLibrary>' | 
                Set-Content '${CMAKE_BINARY_DIR}/${CABBAGE_PROJECT_NAME}.vcxproj';
            } catch {
                Write-Error 'Failed to modify ${CABBAGE_PROJECT_NAME}.vcxproj';
                exit 1;
            }
        "
    )
    target_link_options(${CABBAGE_PROJECT_NAME} PRIVATE "/SUBSYSTEM:WINDOWS")
else()
    target_link_options(${CABBAGE_PROJECT_NAME} PRIVATE LINKER:-adhoc_codesign)
endif()
