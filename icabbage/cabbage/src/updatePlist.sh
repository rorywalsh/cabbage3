#!/bin/bash

# Check if the input argument is provided
if [ $# -ne 1 ]; then
    echo "Usage: $0 <new_string>"
    exit 1
fi

# Input string argument
new_string="$1"

# File path
file_path="$(pwd)/build/CMakeFiles/app.dir/Info.plist"
# Check if file exists
if [ ! -f "$file_path" ]; then
    echo "Error: File not found: $file_path"
    exit 1
fi

# Replace occurrences of "Cabbage" with the new string, excluding instances in <string>Cabbage-macOS-MainMenu</string>
sed -i '' -E "/<string>Cabbage-macOS-MainMenu<\/string>/! s/Cabbage/${new_string}/g" "$file_path"

file_path="$(pwd)/build/CMakeFiles/vst3.dir/Info.plist" 
# Check if file exists
if [ ! -f "$file_path" ]; then
    echo "Error: File not found: $file_path"
    exit 1
fi

# no longer building for vst2.4
# # Replace occurrences of "Cabbage" with the new string, excluding instances in <string>Cabbage-macOS-MainMenu</string>
# sed -i '' -E "/<string>Cabbage-macOS-MainMenu<\/string>/! s/Cabbage/${new_string}/g" "$file_path"

# file_path="$(pwd)/build/CMakeFiles/vst2.dir/Info.plist" 
# # Check if file exists
# if [ ! -f "$file_path" ]; then
#     echo "Error: File not found: $file_path"
#     exit 1
# fi

# Replace occurrences of "Cabbage" with the new string, excluding instances in <string>Cabbage-macOS-MainMenu</string>
sed -i '' -E "/<string>Cabbage-macOS-MainMenu<\/string>/! s/Cabbage/${new_string}/g" "$file_path"

file_path="$(pwd)/build/CMakeFiles/auv2.dir/Info.plist" 
# Check if file exists
if [ ! -f "$file_path" ]; then
    echo "Error: File not found: $file_path"
    exit 1
fi

# Replace occurrences of "Cabbage" with the new string, excluding instances in <string>Cabbage-macOS-MainMenu</string>
sed -i '' -E "/<string>Cabbage-macOS-MainMenu<\/string>/! s/Cabbage/${new_string}/g" "$file_path"

echo "String replaced successfully across plugin and standalone projects."



