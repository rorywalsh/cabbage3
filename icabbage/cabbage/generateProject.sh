#!/bin/bash

rm -rf build

# Check if the correct number of arguments is provided
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <target>"
    exit 1
fi

# Check if the provided target is valid
case "$1" in
    CabbageApp|CabbagePluginEffect|CabbagePluginSynth)
        ;;
    *)
        echo "Invalid target: $1"
        echo "Valid targets are: CabbageApp, CabbagePluginEffect, CabbagePluginSynth"
        exit 1
        ;;
esac

# Set the target variable
target="$1"

# Construct the CMake command with the target variable
command="cmake -GXcode -B build -S . -D${target}=On"

# Print the constructed command
echo "$command"

# Run the constructed command
eval "$command"
./updatePlist.sh $1
