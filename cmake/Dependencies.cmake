include(FetchContent)

# ----------------------------
# Only need these libraries for CabbageServiceApp and CabbageTests
# ----------------------------
if (CabbageApp STREQUAL "${CABBAGE_BUILD_TARGET}" OR CabbageTests STREQUAL "${CABBAGE_BUILD_TARGET}")
    message(STATUS "Including RtAudio/RtMidi/ixWebsocket for ${CABBAGE_BUILD_TARGET}")

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

    # Use FetchContent to include RtMidi
    FetchContent_Declare(
        rtmidi
        GIT_REPOSITORY https://github.com/thestk/rtmidi.git
        GIT_TAG master
    )

    
    # Fetch the IXWebSocket repository
    FetchContent_Declare(
        ixwebsocket
        GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
        GIT_TAG master  # You can specify a branch, tag, or commit hash here
    )

    # Set the BUILD_SHARED_LIBS option for IXWebSocket to OFF to build static library
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static library for IXWebSocket" FORCE)
    # Set the USE_SSL option for IXWebSocket to OFF to disable SSL support
    set(USE_SSL OFF CACHE BOOL "Disabling SSL for ixwebsocket" FORCE)
    set(USE_ZLIB  OFF CACHE BOOL "Disabling zlib for ixwebsocket" FORCE)

    # Make all dependencies available
    FetchContent_MakeAvailable(ixwebsocket rtaudio rtmidi)

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

FetchContent_Declare(
  catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.2  # Latest stable version (adjust as needed)
)

# Make all dependencies available
FetchContent_MakeAvailable(lattice catch2 readerwriterqueue)
