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

if(WIN32)
    # Having to do this here because I can't seem to get it to work during the generation process. 
    # This is an ugly hack, but I'm out of ideas, none of the recommended CMake ways of changing the 
    # runtime library work.
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_custom_command(
            TARGET ${CABBAGE_PROJECT_NAME} PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Modifying ${CABBAGE_PROJECT_NAME}.vcxproj file"
            COMMAND powershell -Command "(Get-Content ${CMAKE_BINARY_DIR}/${CABBAGE_PROJECT_NAME}.vcxproj) -replace '<RuntimeLibrary>MultiThreadedDebug</RuntimeLibrary>', '<RuntimeLibrary>MultiThreadedDebugDLL</RuntimeLibrary>' | Set-Content ${CMAKE_BINARY_DIR}/${CABBAGE_PROJECT_NAME}.vcxproj"
        )
    else()
        add_custom_command(
            TARGET ${CABBAGE_PROJECT_NAME} PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Modifying ${CABBAGE_PROJECT_NAME}.vcxproj file"
            COMMAND powershell -Command "(Get-Content ${CMAKE_BINARY_DIR}/${CABBAGE_PROJECT_NAME}.vcxproj) -replace '<RuntimeLibrary>MultiThreaded</RuntimeLibrary>', '<RuntimeLibrary>MultiThreadedDLL</RuntimeLibrary>' | Set-Content ${CMAKE_BINARY_DIR}/${CABBAGE_PROJECT_NAME}.vcxproj"
        )
    endif()
    target_link_options(${CABBAGE_PROJECT_NAME} PRIVATE "/SUBSYSTEM:WINDOWS")
else()
    target_link_options(${CABBAGE_PROJECT_NAME} PRIVATE LINKER:-adhoc_codesign)
endif()
