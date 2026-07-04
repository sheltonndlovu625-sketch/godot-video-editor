#!/bin/bash
# Run this once after cloning the repo to fetch godot-cpp
set -e

if [ -d "godot-cpp" ]; then
    echo "godot-cpp already exists. Pulling latest..."
    cd godot-cpp
    git pull origin 4.4.1-stable
    cd ..
else
    echo "Cloning godot-cpp 4.4.1-stable..."
    git clone --branch 4.4.1-stable --depth 1         https://github.com/godotengine/godot-cpp.git
fi

echo "godot-cpp ready."
