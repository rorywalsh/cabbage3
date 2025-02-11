#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <input_binary> <output_header> <array_name>"
    exit 1
fi

# Assign arguments to variables
INPUT_BINARY="$1"
OUTPUT_HEADER="$2"
ARRAY_NAME="$3"

# Ensure the input binary file exists
if [ ! -f "$INPUT_BINARY" ]; then
    echo "Error: Input binary file '$INPUT_BINARY' not found."
    exit 1
fi

# Create the output header file with the Base64-encoded data
{
    echo "const char ${ARRAY_NAME}[] = R\"("
    base64 -w 0 "$INPUT_BINARY"
    echo ")\";"
} > "$OUTPUT_HEADER"

echo "Generated Base64-encoded header file: $OUTPUT_HEADER"