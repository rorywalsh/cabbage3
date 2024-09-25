include_guard()

message(STATUS "Fetching dependencies ...")

message(STATUS "Finding vcpkg packages")

# Automatically find packages listed in vcpkg.json's dependencies array.
file(READ "vcpkg.json" vcpkg_json)
string(JSON vcpkg_deps GET ${vcpkg_json} "dependencies")
string(JSON vcpkg_deps_count LENGTH ${vcpkg_deps})
math(EXPR vcpkg_deps_max_index "${vcpkg_deps_count} - 1")
foreach(dep_index RANGE ${vcpkg_deps_max_index})
    string(JSON dep_name GET ${vcpkg_deps} ${dep_index})
    message(STATUS "Finding vcpkg package ${dep_name}")
    find_package(${dep_name} CONFIG REQUIRED)
    message(STATUS "Finding vcpkg package ${dep_name} - done")
endforeach()

message(STATUS "Finding vcpkg packages - done")

include(FetchContent)

message(STATUS "Fetching dependency choc")

FetchContent_Declare(
    choc
    URL https://codeload.github.com/Tracktion/choc/tar.gz/85149958b6d0e51885eefba8816b51798570b54b
    URL_MD5 ffdb942dcaa64a716239d37c620e5eb2
    DOWNLOAD_DIR "${FETCHCONTENT_BASE_DIR}/choc"
    DOWNLOAD_EXTRACT_TIMESTAMP true
)

FetchContent_MakeAvailable(choc)

message(STATUS "Fetching dependency choc - done")

message(STATUS "Fetching dependency iPlug2")

FetchContent_Declare(
    iPlug2
    URL https://codeload.github.com/rorywalsh/iPlug2/tar.gz/baa9a03bf2488a809e9020cd4c1dc69b20b52913
    URL_MD5 fe30ea1822d9935ce9dbd1ef0adb86c2
    DOWNLOAD_DIR "${FETCHCONTENT_BASE_DIR}/iPlug2"
    DOWNLOAD_EXTRACT_TIMESTAMP true
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
    SOURCE_DIR "${iplug2_SOURCE_DIR}/Build"
)

FetchContent_MakeAvailable(iPlug2_dependencies)

message(STATUS "Fetching dependency iPlug2_dependencies - done")

message(STATUS "Fetching dependency iPlug2_vst3_sdk")

FetchContent_Declare(
    iPlug2_vst3_sdk
    GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
    GIT_TAG cc2adc90382dded9e347caf74e4532f1458715db
    GIT_SUBMODULES "base" "cmake" "pluginterfaces" "public.sdk" "vstgui4"
    DOWNLOAD_DIR "${FETCHCONTENT_BASE_DIR}/iPlug2_vst3_sdk"
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
    DOWNLOAD_DIR "${FETCHCONTENT_BASE_DIR}/iPlug2_wam_audioworklet_polyfill"
    SOURCE_DIR "${iplug2_SOURCE_DIR}/Dependencies/iPlug/WAM_AWP"
)

FetchContent_MakeAvailable(iPlug2_wam_audioworklet_polyfill)

message(STATUS "Fetching dependency iPlug2_wam_audioworklet_polyfill - done")

message(STATUS "Fetching dependency readerwriterqueue")

FetchContent_Declare(
    readerwriterqueue
    URL https://github.com/cameron314/readerwriterqueue/archive/refs/tags/v1.0.6.tar.gz
    URL_MD5 a22feb68840ba44cbc2b01853fb4a503
    DOWNLOAD_DIR "${FETCHCONTENT_BASE_DIR}/readerwriterqueue"
    DOWNLOAD_EXTRACT_TIMESTAMP true
)

FetchContent_MakeAvailable(readerwriterqueue)

message(STATUS "Fetching dependency readerwriterqueue - done")

message(STATUS "Fetching dependencies - done")
