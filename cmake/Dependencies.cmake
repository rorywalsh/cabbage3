include(FetchContent)

FetchContent_Declare(
  catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.2 
)

# ----------------------------
# Only need these libraries for CabbageServiceApp and CabbageTests
# ----------------------------
if (CabbageApp STREQUAL "${CABBAGE_BUILD_TARGET}" OR CabbageTests STREQUAL "${CABBAGE_BUILD_TARGET}")
    message(STATUS "Including RtAudio/RtMidi for ${CABBAGE_BUILD_TARGET}")

    # Use FetchContent to include RtAudio first
    FetchContent_Declare(
        rtaudio
        GIT_REPOSITORY https://github.com/thestk/rtaudio.git
        GIT_TAG master
    )

    # Disable unnecessary backends on macOS
    if (APPLE)
        set(RTAUDIO_API_JACK OFF CACHE BOOL "Disable JACK support" FORCE)
        set(RTAUDIO_API_PULSE OFF CACHE BOOL "Disable PulseAudio support" FORCE)
        set(RTAUDIO_API_ALSA OFF CACHE BOOL "Disable ALSA support" FORCE)
        set(RTMIDI_API_JACK OFF CACHE BOOL "Disabling Jack for RtMidi" FORCE)
    endif()

    # Disable testing to avoid conflict with RtAudio
    set(RTMIDI_BUILD_TESTING OFF CACHE BOOL "Disable RtMidi tests" FORCE)

    # Build static libraries for rtaudio and rtmidi
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Force static libraries" FORCE)
    set(RTAUDIO_BUILD_SHARED_LIBRARY OFF CACHE BOOL "Build static library for RtAudio" FORCE)
    set(RTMIDI_BUILD_SHARED_LIBRARY OFF CACHE BOOL "Build static library for RtMidi" FORCE)

    # Use FetchContent to include RtMidi
    FetchContent_Declare(
        rtmidi
        GIT_REPOSITORY https://github.com/thestk/rtmidi.git
        GIT_TAG master
    )

    # Make all dependencies available
    FetchContent_MakeAvailable(rtaudio rtmidi)

    # Only make catch2 available when building tests
    if(CabbageTests STREQUAL "${CABBAGE_BUILD_TARGET}")
        FetchContent_MakeAvailable(catch2)
    endif()

endif()

# Lattice and ReaderWriterQueue Dependencies
set(LATTICE_BUILD_EXAMPLES OFF)
FetchContent_Declare(
    lattice
    GIT_REPOSITORY https://github.com/rorywalsh/lattice.git
    GIT_TAG main
)

FetchContent_Declare(
    readerwriterqueue
    GIT_REPOSITORY https://github.com/rorywalsh/readerwriterqueue.git
    GIT_TAG ab2082837bda45e8a1a2d6934b211212ae3e2d1b
)



# Include choc patches functionality
include(${CMAKE_SOURCE_DIR}/cmake/ChocPatches.cmake)

# Make all dependencies available
FetchContent_MakeAvailable(lattice readerwriterqueue)


# Apply choc patches after lattice (which includes choc) is downloaded
if(APPLE)
    apply_choc_patches()
endif()