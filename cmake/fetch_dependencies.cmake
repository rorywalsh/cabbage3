include_guard()

message(STATUS "Fetching dependencies ...")

include(FetchContent)

message(STATUS "Fetching dependency choc")

FetchContent_Declare(
    choc
    GIT_REPOSITORY https://github.com/Tracktion/choc.git
    GIT_TAG 85149958b6d0e51885eefba8816b51798570b54b
    SOURCE_DIR "${FETCHCONTENT_BASE_DIR}/choc-src/include/choc"
)

FetchContent_MakeAvailable(choc)

message(STATUS "Fetching dependency choc - done")

message(STATUS "Fetching dependency iPlug2")

FetchContent_Declare(
    iPlug2
    GIT_REPOSITORY https://github.com/rorywalsh/iPlug2.git
    GIT_TAG 70234c3f5923586db44e91fa8b4a66a9cb22230e
)

FetchContent_MakeAvailable(iPlug2)

message(STATUS "Fetching dependency iPlug2 - done")

message(STATUS "Fetching dependency iPlug2_dependencies")

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(iPlug2_dependencies_zip_file "IPLUG2_DEPS_MAC")
    set(iPlug2_dependencies_url_md5 b85fadfaba0806ce57b4c4d486453451)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(iPlug2_dependencies_zip_file "IPLUG2_DEPS_WIN")
    # TODO: Update iPlug2 dependencies zip file md5 hash for Windows.
    set(iPlug2_dependencies_url_md5 ef6cec00694e5791e21368483f8f8a65)
else()
    message(FATAL_ERROR "Unsupported system for iPlug2: ${CMAKE_SYSTEM_NAME}")
endif()

# Fetch iPlug2 dependencies into the iPlug2 source directory's Build subdirectory, creating it if needed.
# This replicates the behavior of the iPlug2 download-prebuilt-libs.sh script.
FetchContent_Declare(
    iPlug2_dependencies
    URL https://github.com/iPlug2/iPlug2/releases/download/setup/${iPlug2_dependencies_zip_file}.zip
    URL_MD5 ${iPlug2_dependencies_url_md5}
    DOWNLOAD_DIR "${FETCHCONTENT_BASE_DIR}/iPlug2_dependencies"
    DOWNLOAD_EXTRACT_TIMESTAMP true
    SOURCE_DIR "${iplug2_SOURCE_DIR}/Dependencies/Build"
)

FetchContent_MakeAvailable(iPlug2_dependencies)

message(STATUS "Fetching dependency iPlug2_dependencies - done")

message(STATUS "Fetching dependency iPlug2_vst3_sdk")

# Set these variables to disable building VST3 plugin examples, VST3 hosting examples, and VSTGUI support.
#
# NB: The original iPlug2 script removes the associated directories for these features, but that causes errors when
# using the CMake FetchContent API.
set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF)
set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF)
set(SMTG_ENABLE_VSTGUI_SUPPORT OFF)

FetchContent_Declare(
    iPlug2_vst3_sdk
    GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
    GIT_TAG cc2adc90382dded9e347caf74e4532f1458715db
    GIT_SUBMODULES "base" "cmake" "pluginterfaces" "public.sdk" "vstgui4"
    SOURCE_DIR "${iplug2_SOURCE_DIR}/Dependencies/iPlug/VST3_SDK"
)

FetchContent_MakeAvailable(iPlug2_vst3_sdk)

message(STATUS "Fetching dependency iPlug2_vst3_sdk - done")

message(STATUS "Fetching dependency iPlug2_clap_sdk")

FetchContent_Declare(
    iPlug2_clap_sdk
    GIT_REPOSITORY https://github.com/free-audio/clap.git
    GIT_TAG 27f20f81dec40b930d79ef429fd35dcc2d45db5b
    DOWNLOAD_DIR "${FETCHCONTENT_BASE_DIR}/iPlug2_clap_sdk"
    SOURCE_DIR "${iplug2_SOURCE_DIR}/Dependencies/iPlug/CLAP_SDK"
)

FetchContent_MakeAvailable(iPlug2_clap_sdk)

message(STATUS "Fetching dependency iPlug2_clap_sdk - done")

message(STATUS "Fetching dependency iPlug2_clap_helpers")

FetchContent_Declare(
    iPlug2_clap_helpers
    GIT_REPOSITORY https://github.com/free-audio/clap-helpers.git
    GIT_TAG 59791394dc26637d9425c2745233a979602be2a7
    DOWNLOAD_DIR "${FETCHCONTENT_BASE_DIR}/iPlug2_clap_helpers"
    SOURCE_DIR "${iplug2_SOURCE_DIR}/Dependencies/iPlug/CLAP_HELPERS"
)

FetchContent_MakeAvailable(iPlug2_clap_helpers)

message(STATUS "Fetching dependency iPlug2_clap_helpers - done")

message(STATUS "Fetching dependency iPlug2_wam_sdk")

FetchContent_Declare(
    iPlug2_wam_sdk
    GIT_REPOSITORY https://github.com/iplug2/api.git
    GIT_TAG e0fb276b5a16df4d3b7419b14d6347593ebd8846
    DOWNLOAD_DIR "${FETCHCONTENT_BASE_DIR}/iPlug2_wam_sdk"
    SOURCE_DIR "${iplug2_SOURCE_DIR}/Dependencies/iPlug/WAM_SDK"
)

FetchContent_MakeAvailable(iPlug2_wam_sdk)

message(STATUS "Fetching dependency iPlug2_wam_sdk - done")

message(STATUS "Fetching dependency iPlug2_wam_audioworklet_polyfill")

FetchContent_Declare(
    iPlug2_wam_audioworklet_polyfill
    GIT_REPOSITORY https://github.com/iplug2/audioworklet-polyfill
    GIT_TAG 771ddc51076c7d0c5db6ac773f304a2a85a4a8ae
    SOURCE_DIR "${iplug2_SOURCE_DIR}/Dependencies/iPlug/WAM_AWP"
)

FetchContent_MakeAvailable(iPlug2_wam_audioworklet_polyfill)

message(STATUS "Fetching dependency iPlug2_wam_audioworklet_polyfill - done")

message(STATUS "Fetching dependency readerwriterqueue")

FetchContent_Declare(
    readerwriterqueue
    GIT_REPOSITORY https://github.com/cameron314/readerwriterqueue.git
    GIT_TAG v1.0.6
)

FetchContent_MakeAvailable(readerwriterqueue)

message(STATUS "Fetching dependency readerwriterqueue - done")

message(STATUS "Fetching dependencies - done")
