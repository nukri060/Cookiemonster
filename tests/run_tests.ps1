# Create build directory if it doesn't exist
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build"
}

# Navigate to build directory
Set-Location "build"

# Configure CMake
cmake ..

# Build the project
cmake --build . --config Release

# Run tests
ctest --output-on-failure

# Return to original directory
Set-Location .. 