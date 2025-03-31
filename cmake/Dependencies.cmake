include(FetchContent)

# Use FetchContent to include RtAudio first
# FetchContent_Declare(
#     rtaudio
#     GIT_REPOSITORY https://github.com/thestk/rtaudio.git
#     GIT_TAG master
# )

# # Disable unnecessary backends on macOS
# if (APPLE)
#     set(RTAUDIO_API_JACK OFF CACHE BOOL "Disable JACK support" FORCE)
#     set(RTAUDIO_API_PULSE OFF CACHE BOOL "Disable PulseAudio support" FORCE)
#     set(RTAUDIO_API_ALSA OFF CACHE BOOL "Disable ALSA support" FORCE)
#     set(RTMIDI_API_JACK OFF CACHE BOOL "Disabling Jack for RtMidi" FORCE)
# endif()

# # Disable testing to avoid conflict with RtAudio
# set(RTMIDI_BUILD_TESTING OFF CACHE BOOL "Disable RtMidi tests" FORCE)

# # Use FetchContent to include RtMidi
# FetchContent_Declare(
#     rtmidi
#     GIT_REPOSITORY https://github.com/thestk/rtmidi.git
#     GIT_TAG master
# )


set(LATTICE_BUILD_EXAMPLES Off)
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


# Make all dependencies available
FetchContent_MakeAvailable(lattice readerwriterqueue)
