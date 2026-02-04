# Function to apply patches to clap-wrapper dependency
function(apply_clap_wrapper_patches)
    # Use FETCHCONTENT_BASE_DIR which is shared across multi-target sub-builds,
    # rather than CMAKE_BINARY_DIR which points to per-target subdirectories
    set(CLAP_WRAPPER_SOURCE_DIR "${FETCHCONTENT_BASE_DIR}/clap-wrapper-src")
    set(PATCH_DIR ${CMAKE_SOURCE_DIR}/cmake/patches)
    set(VST3_MIDI_PATCH ${PATCH_DIR}/clap-wrapper-vst3-midi-output.patch)
    set(AUV2_MIDI_PATCH ${PATCH_DIR}/clap-wrapper-auv2-midi-output.patch)

    if(EXISTS ${CLAP_WRAPPER_SOURCE_DIR})
        message(STATUS "Applying clap-wrapper patches to ${CLAP_WRAPPER_SOURCE_DIR}...")

            # Apply VST3 MIDI output patch
            set(PROCESS_CPP ${CLAP_WRAPPER_SOURCE_DIR}/src/detail/vst3/process.cpp)
            set(PROCESS_H ${CLAP_WRAPPER_SOURCE_DIR}/src/detail/vst3/process.h)

            if(EXISTS ${PROCESS_CPP} AND EXISTS ${PROCESS_H} AND EXISTS ${VST3_MIDI_PATCH})
                # Test if patch can be applied
                execute_process(
                    COMMAND patch -p1 -N --dry-run
                    INPUT_FILE ${VST3_MIDI_PATCH}
                    WORKING_DIRECTORY ${CLAP_WRAPPER_SOURCE_DIR}
                    RESULT_VARIABLE PATCH_TEST_RESULT
                    OUTPUT_QUIET
                    ERROR_QUIET
                )

                if(PATCH_TEST_RESULT EQUAL 0)
                    # Apply the patch
                    execute_process(
                        COMMAND patch -p1 -N
                        INPUT_FILE ${VST3_MIDI_PATCH}
                        WORKING_DIRECTORY ${CLAP_WRAPPER_SOURCE_DIR}
                        RESULT_VARIABLE PATCH_RESULT
                        OUTPUT_VARIABLE PATCH_OUTPUT
                        ERROR_VARIABLE PATCH_ERROR
                    )

                    if(PATCH_RESULT EQUAL 0)
                        message(STATUS "  ✓ Applied VST3 MIDI output patch")
                    else()
                        message(WARNING "  ✗ Failed to apply VST3 MIDI output patch: ${PATCH_ERROR}")
                    endif()
                else()
                    message(STATUS "  - VST3 MIDI output patch already applied or not needed")
                endif()
            else()
                if(NOT EXISTS ${VST3_MIDI_PATCH})
                    message(WARNING "  ✗ Patch file not found: ${VST3_MIDI_PATCH}")
                endif()
            endif()

            # Apply AUv2 MIDI output patch (macOS only)
            if(APPLE)
                set(WRAPASAUV2_CPP ${CLAP_WRAPPER_SOURCE_DIR}/src/wrapasauv2.cpp)

                if(EXISTS ${WRAPASAUV2_CPP} AND EXISTS ${AUV2_MIDI_PATCH})
                    # Test if patch can be applied
                    execute_process(
                        COMMAND patch -p1 -N --dry-run
                        INPUT_FILE ${AUV2_MIDI_PATCH}
                        WORKING_DIRECTORY ${CLAP_WRAPPER_SOURCE_DIR}
                        RESULT_VARIABLE PATCH_TEST_RESULT
                        OUTPUT_QUIET
                        ERROR_QUIET
                    )

                    if(PATCH_TEST_RESULT EQUAL 0)
                        # Apply the patch
                        execute_process(
                            COMMAND patch -p1 -N
                            INPUT_FILE ${AUV2_MIDI_PATCH}
                            WORKING_DIRECTORY ${CLAP_WRAPPER_SOURCE_DIR}
                            RESULT_VARIABLE PATCH_RESULT
                            OUTPUT_VARIABLE PATCH_OUTPUT
                            ERROR_VARIABLE PATCH_ERROR
                        )

                        if(PATCH_RESULT EQUAL 0)
                            message(STATUS "  ✓ Applied AUv2 MIDI output patch")
                        else()
                            message(WARNING "  ✗ Failed to apply AUv2 MIDI output patch: ${PATCH_ERROR}")
                        endif()
                    else()
                        message(STATUS "  - AUv2 MIDI output patch already applied or not needed")
                    endif()
                else()
                    if(NOT EXISTS ${AUV2_MIDI_PATCH})
                        message(WARNING "  ✗ Patch file not found: ${AUV2_MIDI_PATCH}")
                    endif()
                endif()
            endif()

        message(STATUS "Clap-wrapper patches applied successfully")
    else()
        message(STATUS "Clap-wrapper source not found yet (will be patched when available)")
    endif()
endfunction()
