# Cabbage3 - Audio Plugin Framework

## Project Overview
Cabbage3 is a framework for developing audio plugins (CLAP, VST3, AUv2) and standalone applications using Csound. It features a modern web-based UI system with JSON widget definitions and real-time parameter automation.

## Architecture

### Core Components

1. **CabbageProcessor** (`src/CabbageProcessor.cpp`)
   - Main audio processor class
   - Inherits from `lattice::Processor`
   - Manages Csound engine lifecycle
   - Handles parameter automation and DAW communication
   - Routes messages between UI and Csound

2. **Engine** (`src/Cabbage.cpp`)
   - Csound engine wrapper
   - Widget JSON parser and validator
   - Opcode handler (create, set, get operations)
   - Channel cache management
   - Csound compilation and error handling

3. **Lattice Framework** (`lattice/src/`)
   - Plugin wrapper library (CLAP, VST3, AUv2)
   - Parameter system with normalization
   - Webview integration (Choc library)
   - Cross-platform audio I/O

4. **Widget System** (JavaScript in webview)
   - Same as VSCabbage extension
   - Renders in plugin webview or standalone window

### Build Modes

**1. Plugin Mode (CLAP/VST3/AUv2)**
- Built as audio plugin
- DAW automation via parameter system
- Webview embedded in plugin window
- Thread architecture: Audio → Idle → Main/UI

**2. Standalone Mode (CabbageApp)**
- Built as desktop application
- No parameter system (direct channels only)
- Stdio communication with VS Code extension
- Thread architecture: Audio → Idle → Stdout

## Threading Model

### Plugin Mode

```
Audio Thread (Csound performKsmps)
    ↓ updateChannelCache()
    ↓ flushChannelCache() [per-block]
Idle Thread (onIdle ~60Hz)
    ↓ dequeue opcodeData
    ↓ updateWidgetData()
    ↓ sendWebViewMessage()
    ↓ enqueue webviewMessageQueue
Main/UI Thread (CLAP timer ~60fps)
    ↓ processWebviewMessages()
    ↓ evaluateJavascript()
Webview (JavaScript)
```

### Standalone Mode

```
Audio Thread (Csound performKsmps)
    ↓ updateChannelCache()
    ↓ flushChannelCache() [per-block]
Idle Thread (onIdle ~60Hz)
    ↓ dequeue opcodeData [only if allowDequeuing=true]
    ↓ hostCallback()
    ↓ sendJsonMessage()
Stdout → VS Code Extension
```

## Message Flow

### Opcode → UI Update

1. **Csound Code**
   ```csound
   cabbageSetValue "slider1", kValue
   ```

2. **Audio Thread** (`Cabbage.cpp`)
   ```cpp
   updateChannelCache("slider1", value)  // Mark dirty
   flushChannelCache()                    // Enqueue to opcodeData
   ```

3. **Idle Thread** (`CabbageProcessor.cpp`)
   ```cpp
   onIdle() {
     while (opcodeData.try_dequeue(data)) {
       updateWidgetData(data);  // Send to webview queue
     }
   }
   ```

4. **Main Thread** (Plugin) or **Stdout** (Standalone)
   ```cpp
   // Plugin
   processWebviewMessages() { webview->evaluateJavascript(...) }
   
   // Standalone
   hostCallback() { sendJsonMessage(stdout) }
   ```

### UI → Csound Update

1. **User Interaction** (JavaScript)
   ```javascript
   // For automatable widgets (sends parameterChange)
   Cabbage.sendParameterUpdate({
     command: "parameterChange",
     paramIdx: 0,
     channel: "slider1",
     value: 0.75
   }, vscode);
   
   // For non-automatable widgets (sends channelData)
   Cabbage.sendChannelData("slider1", 0.75, vscode);
   ```

2. **Message Handler** (`CabbageProcessor.cpp`)
   ```cpp
   processWebViewCommand() {
     // Parse obj field if present (plugin mode)
     setControlChannel("slider1", 0.75)
   }
   ```

3. **Csound Code**
   ```csound
   kValue cabbageGetValue "slider1"
   ```

## Parameter System (Plugin Mode Only)

### Parameter Creation
Automatically created for widgets with `"automatable": true`:

```cpp
void CabbageProcessor::addParametersForWidget(json& w) {
  if (w["automatable"] == true) {
    for (auto& ch : w["channels"]) {
      addParameter({
        ch["id"],           // name
        min, max,           // range
        defaultValue,
        increment,
        skew
      });
    }
  }
}
```

### Parameter Storage
**Critical**: Parameters store **normalized values (0-1)**, not denormalized values.

```cpp
void CabbageProcessor::setParameter(int paramId, double value) {
  getParameters()[paramId].value = value;  // Store normalized!
  
  float denormValue = getParameter(paramId).fromNormalised(value);
  cabbage.setControlChannel(channel, denormValue);  // Csound gets full-range
}
```

### DAW Automation Flow

1. **User drags slider**: `value=82` (full-range)
2. **UI sends**: `{command: "parameterChange", paramIdx: 0, value: 82}`
3. **C++ normalizes**: `82 / 320 = 0.25625`
4. **C++ stores**: `params[0].value = 0.25625`
5. **C++ updates Csound**: `setControlChannel("bpm", 82)`
6. **DAW receives**: `addParameterChange({0, 0.25625, ...})`
7. **DAW sends back**: CLAP event with `normalized=0.25625`
8. **C++ denormalizes**: `0.25625 * 320 = 82`
9. **UI receives**: `{command: "parameterChange", value: 82}`

## Channel Cache System

### Purpose
Prevents flooding the opcode queue with duplicate updates during audio processing.

### Implementation
```cpp
// channelCache stores CabbageOpcodeData structs, not a separate ChannelCacheEntry
std::unordered_map<std::string, CabbageOpcodeData> channelCache;
```

### Operations

**Update Cache** (called by opcodes):
```cpp
void updateChannelCache(string channel, float value) {
  auto& entry = channelCache[channel];
  if (entry.value != value) {
    entry.value = value;
    entry.isDirty = true;
  }
}
```

**Flush Cache** (called once per audio block):
```cpp
void flushChannelCache() {
  for (auto& [channel, entry] : channelCache) {
    if (entry.isDirty) {
      opcodeData.enqueue({channel, entry.value});
      entry.isDirty = false;
    }
  }
}
```

### Timing
- **Audio Thread**: Updates cache thousands of times per second
- **Flush**: Once per block (512 samples @ 44.1kHz = ~86 times/sec)
- **Idle Thread**: Dequeues at ~60Hz
- **Result**: Massive reduction in message traffic

## Csound Opcodes

### cabbageCreate
Creates widgets dynamically at i-time:
```csound
SJson = {{"type": "button", "bounds": {...}, "channels": [...]}}
cabbageCreate SJson
```

**Important**: Now creates Csound channels automatically for all widgets (automatable or not).

### cabbageSetValue
Updates widget value at k-rate:
```csound
cabbageSetValue "widgetId", kValue
```

### cabbageSet
Updates widget properties at k-rate:
```csound
cabbageSet "widgetId", "color", "red"
```

### cabbageGetValue
Reads widget value at k-rate:
```csound
kValue cabbageGetValue "widgetId"
```

### cabbageGet
Reads widget property at k-rate:
```csound
SColor cabbageGet "widgetId", "color"
```

## Widget JSON Schema

### Complete Widget Example
```json
{
  "type": "horizontalSlider",
  "id": "bpmSlider",
  "bounds": {
    "top": 297,
    "left": 10,
    "width": 233,
    "height": 30
  },
  "channels": [{
    "id": "bpmSlider",
    "range": {
      "min": 0,
      "max": 320,
      "defaultValue": 60,
      "increment": 1,
      "skew": 1
    },
    "parameterIndex": 0  // Automatically added during parameter creation
  }],
  "label": {
    "text": "BPM",
    "position": "left"
  },
  "style": {
    "backgroundColor": "#334455",
    "color": "#ffffff"
  },
  "automatable": true  // Creates DAW parameter
}
```

### Widget Types
- **Sliders**: horizontalSlider, verticalSlider, rotarySlider, numberSlider
- **Buttons**: button, checkbox, optionButton
- **Containers**: form, groupbox, image
- **Display**: gentable, label, csoundOutput, keyboard
- **Input**: combobox, listbox, textbox

## Build System

### CMake Configuration
```bash
cd cabbage3
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug
```

### Build Targets
- `CabbageApp` - Standalone application (for VS Code)
- `CabbagePluginEffect` - CLAP/VST3/AUv2 effect plugin
- `CabbagePluginSynth` - CLAP/VST3/AUv2 instrument plugin

### Dependencies (via CPM)
- **Csound** - Audio engine
- **Lattice** - Plugin framework
- **Choc** - Webview library
- **nlohmann/json** - JSON parsing
- **readerwriterqueue** - Lock-free queues

## Key Files

### Core
- `src/CabbageProcessor.cpp` - Main processor
- `src/Cabbage.cpp` - Engine and parser
- `src/CabbageParser.cpp` - JSON validation
- `src/CabbageUtils.cpp` - Utilities

### Opcodes
- `src/opcodes/CabbageCreateOpcode.cpp` - Dynamic widget creation
- `src/opcodes/CabbageSetValueOpcode.cpp` - Value updates
- `src/opcodes/CabbageSetOpcode.cpp` - Property updates
- `src/opcodes/CabbageGetOpcodes.cpp` - Value/property reading

### Standalone
- `src/CabbageAudioApp/CabbageAudioApp.cpp` - Standalone app
- `src/CabbageAudioApp/main.cpp` - Entry point

### Configuration
- `CMakeLists.txt` - Build configuration
- `cmake/Dependencies.cmake` - CPM package management
- `src/CabbagePluginInfo.h` - Plugin metadata

## Common Issues

### Widgets Not Updating from Csound Opcodes
**Symptom**: `cabbageSetValue` doesn't update UI.
**Check**: Ensure `flushChannelCache()` is called once per audio block, not per sample.
**Check**: Verify `allowDequeuing` is enabled when appropriate.

### Widget Interaction Not Reaching Csound
**Symptom**: Moving sliders/clicking buttons doesn't update Csound channels.
**Check**: Verify channel names match between widget JSON and Csound code.
**Check**: For plugin mode, ensure `obj` field is unpacked in `processWebViewCommand()`.

### Compilation Errors Not Displayed
**Symptom**: Plugin shows blank screen instead of error HTML.
**Check**: Error HTML generation in constructor, webview initialization timing.

## Thread Safety

### Lock-Free Queues
All cross-thread communication uses `moodycamel::ReaderWriterQueue`:
- `opcodeData` - Audio → Idle
- `webviewMessageQueue` - Idle → Main
- `parameterChanges` - Any → Audio

### Synchronization Flags
- `allowDequeuing` - Controls when idle thread can process opcodes (not atomic, set during initialization)
- `cabbageIsReady` - Signals webview initialization complete (regular bool, set via mutex-protected method)

### Critical Sections
None! All synchronization via lock-free data structures.

## Debugging

### Enable Debug Logging
```cpp
#define LATTICE_DEBUG 1
```

### Log Levels
- `lattice::logInfo` - General information
- `lattice::logDebug` - Detailed debugging
- `lattice::logError` - Errors and warnings

### Common Debug Points
```cpp
// Channel updates
lattice::logDebug << "Channel: " << channel << " = " << value;

// Parameter changes
lattice::logInfo << "Param " << idx << ": norm=" << norm << ", denorm=" << denorm;

// Thread IDs
lattice::logDebug << "Thread ID: " << std::this_thread::get_id();
```

## Performance Optimization

### Channel Cache
Reduces message traffic by ~1000x:
- Without: Every `cabbageSetValue` call queued (44,100/sec)
- With: Only dirty channels once per block (~86/sec)

### Deduplication
Idle thread keeps only latest value per channel when processing opcodes.

### Webview Message Batching
Main thread processes all queued messages in single JavaScript evaluation.

## Future Enhancements

- [ ] Multi-rate Csound (kr != sr) support
- [ ] MIDI CC parameter mapping
- [ ] Preset management system
- [ ] Undo/redo for widget edits
- [ ] Real-time performance monitoring
- [ ] Remote control via OSC
