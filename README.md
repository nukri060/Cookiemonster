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

## Installation

### Using the Installer

1. Download the latest release from the [releases page](https://github.com/nukri060/Cookiemonster/releases)
2. Run the installer and follow the instructions
3. The program will be installed in `C:\Program Files\CookieMonster`

### Building from Source

```bash
# Clone the repository
git clone https://github.com/nukri060/Cookiemonster.git
cd Cookiemonster

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

## Testing

The project uses Google Test for unit testing. To run the tests:

```bash
# From the build directory
ctest --output-on-failure
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
├── installer/             # Installer scripts
├── CMakeLists.txt         # CMake configuration
└── README.md             # This file
```

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'feat: add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Create a Pull Request

Please follow the [Conventional Commits](https://www.conventionalcommits.org/) specification for commit messages.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Authors

- [Nukri060](https://github.com/nukri060) - Original Author
- Contributors are welcome!

## Version

Current version: 1.0.0 