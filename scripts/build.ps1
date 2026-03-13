# Build Script for Levi ECS Engine

if (!(Test-Path -Path "build")) {
    New-Item -ItemType Directory -Path "build"
}

# Try building with Ninja if available (better for compile_commands.json)
$generator = "Visual Studio 17 2022"
if (Get-Command ninja -ErrorAction SilentlyContinue) {
    $generator = "Ninja"
}

Write-Host "Using generator: $generator" -ForegroundColor Cyan

cmake -B build -S . -G "$generator"
cmake --build build --config Debug

# Copy compile_commands.json to root for Neovim/Clangd
if (Test-Path -Path "build/compile_commands.json") {
    Copy-Item -Path "build/compile_commands.json" -Destination "." -Force
    Write-Host "Copied compile_commands.json to root for LSP." -ForegroundColor Green
} else {
    Write-Warning "compile_commands.json not found. If using MSVC generator, it won't be created."
}

Write-Host "Build complete. Executables in bin/" -ForegroundColor Green
