# Dependency Patches

This directory contains patches applied to third-party dependencies during the build process.

## Current Patches

### 1. `clap-wrapper-vst3-midi-output.patch`

**Status**: Pending upstream contribution
**Upstream**: https://github.com/free-audio/clap-wrapper

**What it fixes**:
- Adds VST3 MIDI output support to the clap-wrapper library
- Converts CLAP MIDI events to VST3 format in `processOutputParams()`
- Handles Note On/Off, Control Change, Pitch Bend, and SysEx events

**Why it's needed**:
The clap-wrapper library currently only handles CLAP→VST3 MIDI *input*, but doesn't translate MIDI *output* from CLAP plugins back to VST3 hosts. This prevents plugins using Csound's `midion2`, `midiout`, etc. from sending MIDI to the host.

**Files modified**:
- `src/detail/vst3/process.h` - Added `_output_events` storage
- `src/detail/vst3/process.cpp` - Implemented event storage and conversion

**Applied by**: `cmake/ClapWrapperPatches.cmake`

**To remove**: When upstream accepts this feature, remove:
1. This patch file
2. The patch application in `ClapWrapperPatches.cmake`
3. Update clap-wrapper version in lattice dependency

---

### 2. `choc_ObjectiveCHelpers.patch`

**Status**: macOS-specific workaround
**Upstream**: https://github.com/Tracktion/choc

**What it fixes**:
- Enhances AUv2 delegate uniqueness to prevent conflicts when multiple plugin instances are loaded

**Applied by**: `cmake/ChocPatches.cmake` (macOS only)

---

## How Patching Works

1. **Fetch dependencies**: CMake downloads dependencies via CPM/FetchContent
2. **Apply patches**: After fetch, `apply_*_patches()` functions run
3. **Dry-run test**: Patches test if they can apply before actually applying
4. **Idempotent**: Safe to run multiple times (skips if already applied)

## Contributing Patches Upstream

### For clap-wrapper VST3 MIDI Output:

1. **Fork the repository**:
   ```bash
   # Fork https://github.com/free-audio/clap-wrapper on GitHub
   git clone https://github.com/YOUR_USERNAME/clap-wrapper.git
   cd clap-wrapper
   ```

2. **Create a feature branch**:
   ```bash
   git checkout -b feature/vst3-midi-output
   ```

3. **Apply the patch**:
   ```bash
   patch -p1 < /path/to/cabbage3/cmake/patches/clap-wrapper-vst3-midi-output.patch
   ```

4. **Test thoroughly**:
   - Build with VST3 support
   - Test Note On/Off, CC, Pitch Bend
   - Verify in multiple DAWs (Reaper, Ableton, Logic, etc.)
   - Test with and without sample-accurate timing

5. **Commit and create PR**:
   ```bash
   git add .
   git commit -m "Add VST3 MIDI output support

   - Convert CLAP MIDI/SysEx events to VST3 format
   - Handle Note On/Off, Control Change, Pitch Bend, etc.
   - Properly route events through processOutputParams()

   This enables CLAP plugins to send MIDI output to VST3 hosts,
   completing the bidirectional MIDI flow."

   git push origin feature/vst3-midi-output
   ```
   Then open a PR on GitHub.

6. **PR Description Template**:
   ```markdown
   ## Summary
   Adds VST3 MIDI output support to clap-wrapper.

   ## Problem
   Currently clap-wrapper handles CLAP→VST3 MIDI input but not output.
   Plugins using CLAP MIDI output (e.g., arpeggiators, MIDI processors)
   cannot send MIDI to VST3 hosts.

   ## Solution
   - Store CLAP MIDI events in `_output_events` vector
   - Convert to VST3 event format in `processOutputParams()`
   - Handle Note On/Off, CC, Pitch Bend, SysEx

   ## Testing
   - Tested with Reaper, Logic Pro, Ableton Live
   - Verified sample-accurate timing
   - Tested various MIDI message types

   ## Breaking Changes
   None - purely additive functionality
   ```

## Removing Patches After Upstream Merge

When upstream accepts the patch:

1. Update lattice to use newer clap-wrapper version
2. Remove patch file from `cmake/patches/`
3. Remove patch application from `cmake/ClapWrapperPatches.cmake`
4. Test clean build to ensure upstream version works
5. Document the version bump in Cabbage release notes

## Adding New Patches

To add a new patch:

1. Make changes to the dependency in `build/*/_deps/<name>-src/`
2. Generate patch:
   ```bash
   cd build/*/_deps/<name>-src
   git diff > /path/to/cabbage3/cmake/patches/<name>-<feature>.patch
   ```
3. Add patch application to appropriate `cmake/*Patches.cmake`
4. Document it in this README
5. Test clean build
