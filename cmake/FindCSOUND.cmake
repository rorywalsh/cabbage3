# Try to find the Csound library.
# Defines the following variables:
#  CSOUND_FOUND - System has the Csound library
#  CSOUND_INCLUDE_DIRS - The Csound include directories.
#  CSOUND_LIBRARIES - The libraries needed to use the Csound library.

if(APPLE)
    find_path(CSOUND_INCLUDE_DIR csound.h
        HINTS
            "$ENV{HOME}/Applications/Csound/CsoundLib64.framework/Headers"
            /Applications/Csound/CsoundLib64.framework/Headers
            ${CSOUND_INCLUDE_DIR_HINT}
    )

    find_path(CSOUND_FRAMEWORK CsoundLib64
        HINTS
            "$ENV{HOME}/Applications/Csound/CsoundLib64.framework"
            /Applications/Csound/CsoundLib64.framework
            ${CSOUND_FRAMEWORK_DIR_HINT}
    )

    find_library(CSOUND_LIBRARY
        NAMES CsoundLib64
        HINTS
            /Applications/Csound/CsoundLib64.framework/
            "$ENV{HOME}/Applications/Csound/CsoundLib64.framework"
            ${CSOUND_LIBRARY_DIR_HINT}
    )

else()

    find_path(CSOUND_INCLUDE_DIR csound.h
        HINTS "C:\\Program Files\\Csound7\\include\\csound"
        ${CSOUND_INCLUDE_DIR_HINT}
    )

    find_library(CSOUND_LIBRARY 
        NAMES csound64 
        HINTS 
            "c:\\Program Files\\Csound6_x64\\lib")

endif()


include(FindPackageHandleStandardArgs)

# Handle the QUIETLY and REQUIRED arguments and set CSOUND_FOUND to TRUE if all listed variables are TRUE.ß
find_package_handle_standard_args(CSOUND
    CSOUND_LIBRARY CSOUND_INCLUDE_DIR
)

mark_as_advanced(CSOUND_INCLUDE_DIR CSOUND_LIBRARY)

set(CSOUND_INCLUDE_DIRS ${CSOUND_INCLUDE_DIR})
set(CSOUND_LIBRARIES ${CSOUND_LIBRARY})