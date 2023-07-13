#!/bin/bash

# Change to the build directory
cd build

# Loop through all files in the directory
for file in *; do
    # Check if the file is a regular file
    if [ -f "$file" ]; then
        # Delete the file
        rm "$file"
    fi
    #check if the file is a directory
    if [ -d "$file" ]; then
        # Delete the directory
        rm -r "./$file"
    fi
done