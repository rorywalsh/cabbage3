#!/bin/bash

# Clear the build directory
rm -rf build

# Determine the operating system
OS_NAME=$(uname -s)

# Check if the correct number of arguments is provided
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <target>"
    echo "Valid targets are: CabbageApp, CabbagePluginEffect, CabbagePluginSynth"
    exit 1
fi

# Validate the provided target
case "$1" in
    CabbageApp|CabbagePluginEffect|CabbagePluginSynth)
        # Set the target variable
        target="$1"
        ;;
    *)
        echo "Invalid target: $1"
        echo "Valid targets are: CabbageApp, CabbagePluginEffect, CabbagePluginSynth"
        exit 1
        ;;
esac

# Construct the CMake command based on the OS
if [ "$OS_NAME" = "Darwin" ]; then
    # macOS specific command
    command="cmake -GXcode -B build -S . -D${target}=On -DCMAKE_BUILD_TYPE=Debug"
    
    echo "$command"
    # Run the constructed CMake command
    eval "$command"
    
    # Check if the CMake command was successful
    if [ $? -ne 0 ]; then
        echo "CMake command failed on macOS."
        exit 1
    fi
    
    # Run updatePlist.sh with the target as an argument
    ./updatePlist.sh "$target"
    
    # Check if the updatePlist.sh script was successful
    if [ $? -ne 0 ]; then
        echo "updatePlist.sh script failed."
        exit 1
    fi

elif [ "$OS_NAME" = "Linux" ]; then
    # TODO: Add Linux-specific support
    echo "Linux support not yet implemented."
    exit 1

else
    # Default command for other systems
    command="cmake -B build -S . -D${target}=On -DPLUG_NAME=${target} -DCMAKE_BUILD_TYPE=Debug"
    
    echo "$command"
    # Run the constructed CMake command
    eval "$command"
    
    # Check if the CMake command was successful
    if [ $? -ne 0 ]; then
        echo "CMake command failed on unsupported OS: $OS_NAME."
        exit 1
    fi
fi

echo "Build completed successfully for target: $target"
