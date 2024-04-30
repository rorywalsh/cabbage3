# cabbage3

This repo contains the Cabbage3 project, and its corresponding vscode extension project. 



1) custom UI webview editor `cabbage/WebView` - this is a modified version of the one that ships with iPlug
2) custom APP target `cabbage/cabbageApp/` (Cmake option: -DCUSTOM_APP_SRC=On) - modified iPlug source with extra functionality

Patches need to be applied to iPlug source to allow for the above changes. The patch files is in the repo root and is called `iplug.patch`

`cd ICabbage`
`cmake -GXcode -S . -B build -DCMAKE_BUILD_TYPE=Debug`