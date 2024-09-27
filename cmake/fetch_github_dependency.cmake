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
        # Unused if USE_GIT is set.
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
            execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory "${FETCHCONTENT_BASE_DIR}/${name}-build")
            execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory "${FETCHCONTENT_BASE_DIR}/${name}-subbuild")
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
            execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory "${FETCHCONTENT_BASE_DIR}/${name}-build")
            execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory "${FETCHCONTENT_BASE_DIR}/${name}-subbuild")
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
