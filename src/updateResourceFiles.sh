#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <old_string> <new_string>"
    exit 1
fi

# Directory path (relative to the script location)
script_dir="$(dirname "$0")"
directory_path="$script_dir/resources"

# Check if directory exists
if [ ! -d "$directory_path" ]; then
    echo "Error: Directory not found: $directory_path"
    exit 1
fi

# Old and new strings
old_string="$1"
new_string="$2"

# Iterate over files starting with the old string
for file in "$directory_path"/"${old_string}"-*; do
    if [ -f "$file" ]; then
        # Extract the file name without extension
        filename="${file%.*}"
        # Get the file extension
        extension="${file##*.}"
        # Generate a new name
        new_name="${filename/${old_string}/${new_string}}"
        # Rename the file
        mv "$file" "$new_name.$extension"
        echo "Renamed $file to $new_name.$extension"
    fi
done
