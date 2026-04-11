#!/bin/bash

# Build Script for Levi ECS Engine (Linux/macOS)

# Function to install dependencies on Linux or macOS
install_deps() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        echo -e "\033[0;36mInstalling system dependencies for Linux...\033[0m"
        sudo apt-get update
        sudo apt-get install -y build-essential git cmake libx11-dev libxext-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libwayland-dev libxkbcommon-dev
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        echo -e "\033[0;36mChecking dependencies for macOS...\033[0m"
        if ! command -v brew >/dev/null 2>&1; then
            echo -e "\033[0;33mHomebrew not found. Please install it from https://brew.sh/ to automate dependency installation.\033[0m"
            exit 1
        fi
        echo -e "\033[0;36mInstalling cmake and ninja via Homebrew...\033[0m"
        brew install cmake ninja
    else
        echo -e "\033[0;33mDependency installation is not supported on this OS via this script.\033[0m"
    fi
}

# Check for --deps flag
if [[ "$1" == "--deps" ]]; then
    install_deps
fi

# Create build directory if it doesn't exist
mkdir -p build

# Determine generator (prefer Ninja if available)
GENERATOR="Unix Makefiles"
if command -v ninja >/dev/null 2>&1; then
    GENERATOR="Ninja"
fi

echo -e "\033[0;36mUsing generator: $GENERATOR\033[0m"

# Configure and build
cmake -B build -S . -G "$GENERATOR" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build --config Debug

# Copy compile_commands.json to root for LSP (Clangd/Neovim)
if [ -f "build/compile_commands.json" ]; then
    cp build/compile_commands.json .
    echo -e "\033[0;32mCopied compile_commands.json to root for LSP.\033[0m"
else
    echo -e "\033[0;33mWarning: compile_commands.json not found.\033[0m"
fi

echo -e "\033[0;32mBuild complete. Executables in bin/\033[0m"
