# CookieMonster 🍪

A modern system cleaning utility for Windows with a focus on performance and user experience.

## 📋 Current Status

Project is in early development stage. Basic functionality for cleaning temporary files and recycle bin is implemented.

## ✨ Features

### Implemented
- Temporary files cleaning
- Recycle bin cleaning
- Administrator rights check
- Cleaning statistics
- User-friendly console interface

### In Development
- Browser cache cleaning
- Registry cleaning
- Modern GUI interface
- Scheduled cleaning tasks
- Detailed cleaning reports

## 🛠️ Requirements

- Windows 10 or newer
- Administrator rights (for full functionality)
- Visual Studio 2022 with C++ development tools (for building)
- CMake 3.15 or newer
- NSIS 3.0 or later (for creating installer)

## 🚀 Building

1. Clone the repository:
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

4. Build project:
```bash
cmake --build . --config Release
```

## 💻 Usage

Run the `systemcleaner.exe` with administrator privileges for full system access:
```bash
./Release/systemcleaner.exe
```

## ⚠️ Security

It is recommended to create a system restore point before using the application.

## 📝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🤝 Support

If you find this project useful, please consider giving it a star ⭐ 

## 📦 Installation

### Using Installer (Recommended)
1. Download the latest `CookieMonster_Setup.exe`
2. Run the installer as administrator
3. Follow the installation wizard
4. Launch CookieMonster from the Start Menu

### Manual Build
1. Clone the repository
2. Create a build directory:
```bash
mkdir build
cd build
```
3. Generate Visual Studio solution:
```bash
cmake ..
```
4. Build the project:
```bash
cmake --build . --config Release
```

The executable will be created in `build/Release/cookiemonster.exe`

## 📊 Usage

1. Run the program as administrator for full functionality
2. Select an option from the menu:
   - Clean Temporary Files
   - Clean Recycle Bin
   - Clean All
   - Exit
3. View cleaning statistics after each operation

## 📦 Building Installer

To create the installer:
1. Install NSIS from https://nsis.sourceforge.io/Download
2. Run `build_installer.bat`
3. The installer will be created as `CookieMonster_Setup.exe`

## 🚨 Warning

This program deletes files permanently. Use with caution. Files deleted cannot be recovered.

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details. 