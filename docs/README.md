# CookieMonster

A Windows utility for cleaning temporary files and system maintenance.

## Features

- Clean temporary files
- Empty recycle bin
- Check administrator rights
- Show cleaning statistics
- Simple console interface

## Requirements

- Windows 10 or later
- Administrator rights
- Visual Studio 2022 with C++ tools
- CMake 3.15 or later
- NSIS 3.0 or later (for installer)

## Installation

### Using Installer
1. Download `CookieMonster_Setup.exe` from [Releases](https://github.com/nukri060/Cookiemonster/releases)
2. Run installer as administrator
3. Follow installation steps
4. Launch from Start Menu

### Manual Build
1. Clone repository:
```bash
git clone https://github.com/nukri060/Cookiemonster.git
cd Cookiemonster
```

2. Create build directory:
```bash
mkdir build
cd build
```

3. Generate project:
```bash
cmake ..
```

4. Build:
```bash
cmake --build . --config Release
```

## Usage

1. Run program as administrator
2. Select option from menu:
   - Clean Temporary Files
   - Clean Recycle Bin
   - Clean All
   - Exit
3. View statistics after cleaning

## Project Structure

```
Cookiemonster/
├── src/                    # Source code
│   ├── include/           # Header files
│   └── source/            # Implementation files
├── docs/                  # Documentation
├── installer/             # Installer files
└── tests/                 # Test files
```

## Building Installer

1. Install NSIS from [nsis.sourceforge.io](https://nsis.sourceforge.io/Download)
2. Run `installer/build_installer.bat`
3. Installer will be created as `CookieMonster_Setup.exe`

## Contributing

1. Fork repository
2. Create feature branch
3. Commit changes
4. Push to branch
5. Create Pull Request

## License

This project is licensed under the MIT License - see [LICENSE](LICENSE) file for details. 