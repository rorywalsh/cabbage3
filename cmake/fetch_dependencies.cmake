include_guard()

# The fetch_github_dependency macro fetches a dependency from a GitHub repository or a .tar.gz file.
#
# Example usage:
#[[
    fetch_github_dependency(
        choc

        GIT_REPOSITORY https://github.com/Tracktion/choc.git

        # When changing the GIT_TAG, the URL_MD5 must be updated to the new expected value.
        GIT_TAG 85149958b6d0e51885eefba8816b51798570b54b

        # Comment out this line to fail the MD5 check and print the expected value.
        URL_MD5 ffdb942dcaa64a716239d37c620e5eb2

        SOURCE_DIR "${FETCHCONTENT_BASE_DIR}/choc-src/include/choc"

        # Uncomment this line to use the Git repository instead of the Github .tar.gz.
        # This makes development easier but slows the fetch down alot, so don't commit it uncommented.
        # USE_GIT
    )
]]
macro(fetch_github_dependency name)
    string(TOLOWER "${name}" name)
    message(TRACE "Macro: fetch_github_dependency(${name}) ...")

    cmake_parse_arguments(ARG "USE_GIT" "GIT_REPOSITORY;GIT_TAG;URL;URL_MD5;SOURCE_DIR" "" ${ARGN})
    message(TRACE "Macro: fetch_github_dependency(${name}) - ARG_USE_GIT = ${ARG_USE_GIT}")
    message(TRACE "Macro: fetch_github_dependency(${name}) - ARG_GIT_REPOSITORY = ${ARG_GIT_REPOSITORY}")
    message(TRACE "Macro: fetch_github_dependency(${name}) - ARG_GIT_TAG = ${ARG_GIT_TAG}")
    message(TRACE "Macro: fetch_github_dependency(${name}) - ARG_URL_MD5 = ${ARG_URL_MD5}")

    if(NOT ARG_SOURCE_DIR)
        set(ARG_SOURCE_DIR "${FETCHCONTENT_BASE_DIR}/${name}-src")
    endif()
    message(TRACE "Macro: fetch_github_dependency(${name}) - ARG_SOURCE_DIR = ${ARG_SOURCE_DIR}")

    if(${ARG_USE_GIT})
        if(DEFINED IS_CABBAGE_CI_BUILD)
            message(FATAL_ERROR "USE_GIT is not supported for CI builds. Comment it out for dependency \"${name}\" to use the Github .tar.gz instead of git clone.")
        endif()

        if(NOT ${name}_USE_GIT AND EXISTS "${ARG_SOURCE_DIR}")
            message(TRACE "Macro: fetch_github_dependency(${name}) - Removing ${ARG_SOURCE_DIR} to fetch the dependency from the Git repository.")
            execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory "${ARG_SOURCE_DIR}")
        endif()
        set(${name}_USE_GIT true CACHE INTERNAL "" FORCE)

        FetchContent_Declare(
            ${name}
            GIT_REPOSITORY "${ARG_GIT_REPOSITORY}"
            GIT_TAG "${ARG_GIT_TAG}"
            SOURCE_DIR "${ARG_SOURCE_DIR}"
        )
    else()
        if(NOT ARG_URL)
            # Convert the git repository url and tag to the .tar.gz url provided by GitHub.
            string(REGEX REPLACE "^https://(.+)/(.+).git/(.*)$" "https://codeload.\\1/\\2/tar.gz/\\3" ARG_URL "${ARG_GIT_REPOSITORY}/${ARG_GIT_TAG}")
        endif()
        message(TRACE "Macro: fetch_github_dependency(${name}) - ARG_URL = ${ARG_URL}")

        # If the URL_MD5 argument is not provided, set it to 1 to fail the check and print the expected value.
        if(NOT ARG_URL_MD5)
            set(ARG_URL_MD5 1)
        endif()

        if(${name}_USE_GIT AND EXISTS "${ARG_SOURCE_DIR}")
            message(TRACE "Macro: fetch_github_dependency(${name}) - Removing ${ARG_SOURCE_DIR} to fetch the dependency from the Github .tar.gz.")
            execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory "${ARG_SOURCE_DIR}")
        endif()
        set(${name}_USE_GIT false CACHE INTERNAL "" FORCE)

        FetchContent_Declare(
            ${name}
            URL "${ARG_URL}"
            URL_MD5 "${ARG_URL_MD5}"
            SOURCE_DIR "${ARG_SOURCE_DIR}"
            DOWNLOAD_DIR "${FETCHCONTENT_BASE_DIR}/${name}"
            DOWNLOAD_EXTRACT_TIMESTAMP true
        )
    endif()

    FetchContent_MakeAvailable(${name})

    message(TRACE "Macro: fetch_github_dependency(${name}) - done")
endmacro()

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
    # USE_GIT
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
