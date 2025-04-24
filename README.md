# CookieMonster

A Windows system cleanup and maintenance tool written in C++.

## Features

- Clean temporary files with size tracking
- Clean browser cache (Chrome, Edge, Firefox)
- Clean Windows registry
- Empty recycle bin
- Administrator privileges check
- Detailed cleaning statistics

## Requirements

- Windows 10 or later
- Visual Studio 2019 or later with C++17 support
- CMake 3.15 or later

## Building

### Using CMake

```bash
# Create build directory
mkdir build
cd build

# Configure and generate build files
cmake ..

# Build the project
cmake --build . --config Release
```

The executable will be created in the `build/bin/Release` directory.

## Usage

Run the program with administrator privileges:

```bash
CookieMonster.exe
```

## Project Structure

```
CookieMonster/
├── src/
│   ├── include/           # Header files
│   │   └── Cleaner.h
│   └── source/            # Source files
│       └── Cleaner.cpp
├── tests/                 # Test files
├── docs/                  # Documentation
├── CMakeLists.txt         # CMake configuration
└── README.md             # This file
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Authors

- Original Author: [Your Name]
- Contributors: [List of contributors] 