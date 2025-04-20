# CookieMonster

A system cleaning utility with a modern interface.

## Current Status

Project is in early development stage.

## Features

- Temporary files cleaning
- Recycle bin cleaning
- Browser cache cleaning (in development)
- Registry cleaning (in development)

## Requirements

- Windows 10 or newer
- Visual Studio 2019 or newer
- CMake 3.10 or newer

## Building

1. Create build directory:
```bash
mkdir build
cd build
```

2. Generate project:
```bash
cmake ..
```

3. Build project:
```bash
cmake --build . --config Release
```

## Usage

Run the `systemcleaner.exe` with administrator privileges for full system access.

## Security

It is recommended to create a system restore point before using the application.

## License

This project is licensed under the MIT License - see the LICENSE file for details. 