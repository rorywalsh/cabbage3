include_guard()

message(STATUS "Fetching dependencies ...")

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
    URL https://codeload.github.com/docEdub/cabbage3-iPlug2-fork/tar.gz/ce184afb2a432aff87e7af6b09b78ec4fb1ad9a3
    URL_MD5 ef6cec00694e5791e21368483f8f8a65
    DOWNLOAD_DIR "${FETCHCONTENT_BASE_DIR}/iPlug2"
    DOWNLOAD_EXTRACT_TIMESTAMP true
)

FetchContent_MakeAvailable(iPlug2)

message(STATUS "Fetching dependency iPlug2 - done")

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
