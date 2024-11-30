include_guard()

include("${CMAKE_CURRENT_LIST_DIR}/fetch_github_dependency.cmake")

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
    GIT_REPOSITORY https://github.com/rorywalsh/iplug2.git
    GIT_TAG 00bd025501615ce5e4ad3e9e4d1e524fd934a82c
    URL_MD5 0560f352162a859d1807aa8760efebe6
    # USE_GIT
)


if(WIN32) 
    # Specify the output directory for NuGet packages (relative to the project root)
    set(NUGET_DOWNLOAD_DIR "${CMAKE_SOURCE_DIR}/packages")

    # Create the packages directory
    file(MAKE_DIRECTORY ${NUGET_DOWNLOAD_DIR})

    # Specify the packages and their versions
    set(NUGET_PACKAGES
        "Microsoft.Web.WebView2/1.0.2478.35"
        "Microsoft.Windows.ImplementationLibrary/1.0.240122.1"
    )


    # Download and extract NuGet packages
    foreach(package_and_version IN LISTS NUGET_PACKAGES)
        # Parse package name and version
        string(REPLACE "/" ";" package_split ${package_and_version})
        list(GET package_split 0 package_name)
        list(GET package_split 1 package_version)

        # Set file paths
        set(package_url "https://www.nuget.org/api/v2/package/${package_name}/${package_version}")
        set(package_file "${NUGET_DOWNLOAD_DIR}/${package_name}.${package_version}.nupkg")
        set(package_extract_dir "${NUGET_DOWNLOAD_DIR}/${package_name}-${package_version}")

        # Download the package if it doesn't already exist
        if (NOT EXISTS "${package_file}")
            message(STATUS "Downloading ${package_name} version ${package_version} from ${package_url}")
            file(DOWNLOAD ${package_url} ${package_file} SHOW_PROGRESS)
            message(STATUS "Downloaded ${package_name} to ${package_file}")
        else()
            message(STATUS "${package_name} version ${package_version} already downloaded")
        endif()

        # Extract the package if it hasn't been extracted yet
        if (NOT EXISTS "${package_extract_dir}")
            message(STATUS "Extracting ${package_name} to ${package_extract_dir}")
            file(MAKE_DIRECTORY "${package_extract_dir}")
            file(ARCHIVE_EXTRACT INPUT ${package_file} DESTINATION "${package_extract_dir}")
            message(STATUS "Extracted ${package_name}")
        else()
            message(STATUS "${package_name} already extracted")
        endif()
    endforeach()

    message(STATUS "All NuGet packages have been downloaded to ${NUGET_DOWNLOAD_DIR}")
endif()

message(DEBUG "Fetching iPlug2 dependencies")

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(iplug2_dependencies_zip_file "IPLUG2_DEPS_MAC")
    set(iplug2_dependencies_url_md5 b85fadfaba0806ce57b4c4d486453451)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(iplug2_dependencies_zip_file "IPLUG2_DEPS_WIN")
    # TODO: Update iPlug2 dependencies zip file md5 hash for Windows.
    set(iplug2_dependencies_url_md5 951BD51FF2F82470624C44AA5400B6D2)
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

message(DEBUG "Fetching iPlug2 dependencies - done")


if(vst3 STREQUAL "${CABBAGE_BUILD_PLUGIN_TYPE}")
    message(DEBUG "Fetching vst3sdk")

    set(vst3sdk_SOURCE_DIR "${iplug2_SOURCE_DIR}/Dependencies/iPlug/VST3_SDK")
    if(NOT EXISTS "${vst3sdk_SOURCE_DIR}/.git" AND EXISTS "${FETCHCONTENT_BASE_DIR}/vst3sdk-build")
        message(TRACE "VST3 SDK source directory not found. Removing fetched dependencies to re-fetch the VST3 SDK.")
        execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory "${FETCHCONTENT_BASE_DIR}/vst3sdk-build")
        execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory "${FETCHCONTENT_BASE_DIR}/vst3sdk-subbuild")
    endif()

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
        SOURCE_DIR "${vst3sdk_SOURCE_DIR}"
    )

    FetchContent_MakeAvailable(vst3sdk)

    message(DEBUG "Fetching vst3sdk - done")
endif()

message(STATUS "Fetching dependencies - done")
