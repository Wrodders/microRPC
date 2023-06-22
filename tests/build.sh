#!/bin/bash

# Create a build directory
mkdir build
cd build

# Run CMake with Ninja
cmake -G Ninja ..

# Build the project
ninja 