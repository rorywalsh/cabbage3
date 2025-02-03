#!/bin/bash
# webview_wrapper.sh

# Set environment variables
export X11_WINDOW_ID=$1
export WIDTH=$2
export HEIGHT=$3
export IS_TRANSPARENT=$4

# Load the .so file and call webview_main
LD_PRELOAD=/home/rory/.vst3/CabbageVST3Effect.vst3/Contents/x86_64-linux/CabbageVST3Effect.so /path/to/a/dummy/executable --webview