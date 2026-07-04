#!/bin/bash
set -e
cd "$(dirname "$0")/.."
if [ ! -d "godot-cpp/.git" ]; then
    echo "Fetching godot-cpp submodule..."
    git submodule update --init --recursive
else
    echo "godot-cpp already present."
fi
echo "godot-cpp ready."
