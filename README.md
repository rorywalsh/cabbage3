# Cabbage3

This repository contains the **Cabbage3** project, along with its corresponding **VSCode extension** project.

### Build Instructions for iPlug Projects

```bash
git clone https://github.com/rorywalsh/cabbage3.git
cd cabbage3
git submodule init
git submodule update
cd icabbae/IXWebSocket
mkdir build && cd build
make
cd ../../iPlug2
git checkout cmake
cd Dependencies
./download-prebuilt-libs.sh
cd IPlug
./download-vst3-sdk.sh
./download-iplug-sdks.sh
cd ../../
git apply --reject ../iplug2.patch
cd ../cabbage
./generateProject.sh CabbagePluginEffect
```

The `generateProject.sh` script will call the relevant CMake scripts to generate either a `CabbagePluginEffect`, `CabbagePluginSynth`, or `CabbageApp` target:

- **CabbagePluginEffect** and **CabbagePluginSynth** are plugin targets.
- **CabbageApp** is the target used when working with the **VSCode extension**. It’s a customized version of the standalone targets for `CabbagePluginEffect` and `CabbagePluginSynth`.

Additionally, on macOS, the `generateProject.sh` script will modify the resulting `info.plist` files with the relevant target name. On MacOS, the CMake build expects to find `CsoundLib64.framework` in `/Library`. On Windows the expected location is `C:/Program Files/Csound6_x64`. There are some extra libraries needed on Windows such as zlib, but these are easily installed and found with vcpgk.

### Project Structure

The source tree, in addition to the usual iPlug2 files and classes, includes modified **WebView** sources (based on `iPlug2/IPlug/Web`) and modified **App** sources (based on `iPlug2/IPlug/App`). These are included via the following CMake defines:

- Custom WebView sources: `CUSTOM_EDITOR` and `CUSTOM_EDITOR_SRC`
- Custom App source: `-DCUSTOM_APP_SRC=On`

The custom App source is patched through the `App.cmake` file.

### Testing the `CabbagePluginEffect` Project

To test the `CabbagePluginEffect` project:

1. Copy the contents of the `CabbagePluginEffect` folder from the top-level `examples` folder to `~/Library/CabbageAudio/CabbagePluginEffect`.
2. This folder contains up-to-date examples, including a copy of the relevant JavaScript source files.

Cabbage3 plugins will search the user library for a `CabbageAudio` folder and look for a folder that matches the plugin binary name. It will then load the corresponding assets, Csound (`.csd`) file, etc. The standalone target `app` is the simplest one to debug.

### Running the VSCode Extension

The **VSCode extension** lives in the `vscabbage` folder. To set it up:

1. Navigate to the `vscabbage` folder:
    ```bash
    cd vscabbage
    ```

2. Install the required dependencies:
    ```bash
    npm install
    ```

To run the extension, click the **debug** button in VSCode.

Once the Cabbage extension is launched, every time you save a file, the extension will try to open **CabbageApp** in the background. The Cabbage settings dialog allows you to specify where to look for this file. If it finds the file, it will launch the app in the background and pass the current file path to it on startup.

- To enter edit mode in CabbageApp, press **Cmd/Ctrl+E**. This stops the performance and gives you access to the **UI Designer**.

### Shared Code and Assets

All the code for the VSCode extension lives in `vscabbage/src`. The plugin interfaces and the VSCode extension share the following files and folders:

- `main.js`
- `utils.js`
- `cabbage.js`
- `widgets/` folder

If you make changes to any of these files before testing a plugin, run the top-level script to copy the updated files to the user library:
```bash
./copySrc.sh
```
This will copy the updated files to `~/Library/CabbageAudio/CabbagePluginEffect`.

