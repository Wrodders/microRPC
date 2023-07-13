#!/bin/bash

# Change to the build/bin directory
cd build/bin

# Loop through all files in the directory
for file in *; do
    # Check if the file is executable
    if [ -x "$file" ]; then
        # Run the file
        "./$file"
    fi
done