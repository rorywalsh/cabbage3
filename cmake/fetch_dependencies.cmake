include_guard()
include("${CMAKE_CURRENT_LIST_DIR}/fetch_dependencies_utils.cmake")

message(STATUS "Fetching dependencies ...")

include(FetchContent)

fetch_github_dependency(
    choc
    GIT_REPOSITORY https://github.com/Tracktion/choc.git
    GIT_TAG 85149958b6d0e51885eefba8816b51798570b54b
    URL_MD5 ffdb942dcaa64a716239d37c620e5eb2
    SOURCE_DIR "${FETCHCONTENT_BASE_DIR}/choc-src/include/choc"
    # USE_GIT
)

fetch_github_dependency(
    readerwriterqueue
    GIT_REPOSITORY https://github.com/cameron314/readerwriterqueue.git
    GIT_TAG v1.0.6
    URL_MD5 a22feb68840ba44cbc2b01853fb4a503
    # USE_GIT
)

fetch_github_dependency(
    iplug2
    GIT_REPOSITORY https://github.com/rorywalsh/iPlug2.git
    GIT_TAG baa9a03bf2488a809e9020cd4c1dc69b20b52913
    URL_MD5 fe30ea1822d9935ce9dbd1ef0adb86c2
    USE_GIT
)

message(STATUS "Fetching dependency iplug2_dependencies")

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(iplug2_dependencies_zip_file "IPLUG2_DEPS_MAC")
    set(iplug2_dependencies_url_md5 b85fadfaba0806ce57b4c4d486453451)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(iplug2_dependencies_zip_file "IPLUG2_DEPS_WIN")
    # TODO: Update iPlug2 dependencies zip file md5 hash for Windows.
    set(iplug2_dependencies_url_md5 ef6cec00694e5791e21368483f8f8a65)
else()
    message(FATAL_ERROR "Unsupported system for iPlug2: ${CMAKE_SYSTEM_NAME}")
endif()

# Fetch iPlug2 dependencies into the iPlug2 source directory's Dependencies/Build subdirectory, creating it if needed.
# This replicates the behavior of the iPlug2 download-prebuilt-libs.sh script.
FetchContent_Declare(
    iplug2_dependencies
    URL https://github.com/iPlug2/iPlug2/releases/download/setup/${iplug2_dependencies_zip_file}.zip
    URL_MD5 ${iplug2_dependencies_url_md5}
    DOWNLOAD_DIR "${FETCHCONTENT_BASE_DIR}/iplug2_dependencies"
    DOWNLOAD_EXTRACT_TIMESTAMP true
    SOURCE_DIR "${iplug2_SOURCE_DIR}/Dependencies/Build"
)

FetchContent_MakeAvailable(iplug2_dependencies)

message(STATUS "Fetching dependency iplug2_dependencies - done")


if(vst3 STREQUAL "${CABBAGE_BUILD_PLUGIN_TYPE}")
    message(STATUS "Fetching dependency vst3sdk")

    # Set these variables to disable building the VST3 SDK plugin examples, hosting examples, and gui support.
    # NB: The original iPlug2 script deletes the associated directories for these features after cloning, but this
    # causes errors when using the CMake FetchContent API, so we do the same thing by setting these variables to OFF.
    set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF)
    set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF)
    set(SMTG_ENABLE_VSTGUI_SUPPORT OFF)

    # Fetch the VST3 SDK from the Git repository instead of the Github .tar.gz because we need its submodules and the
    # .tar.gz doesn't have them.
    FetchContent_Declare(
        vst3sdk
        GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
        GIT_TAG cc2adc90382dded9e347caf74e4532f1458715db
        GIT_SUBMODULES "base" "cmake" "pluginterfaces" "public.sdk" "vstgui4"
        SOURCE_DIR "${iplug2_SOURCE_DIR}/Dependencies/iPlug/VST3_SDK"
    )

    FetchContent_MakeAvailable(vst3sdk)

    message(STATUS "Fetching dependency vst3sdk - done")
endif()

message(STATUS "Fetching dependencies - done")
