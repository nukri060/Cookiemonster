# CookieMonster 🍪

A modern system cleaning utility for Windows with a focus on performance and user experience.

## 📋 Current Status

Project is in early development stage. Basic functionality for cleaning temporary files and recycle bin is implemented.

## ✨ Features

### Implemented
- Temporary files cleaning
- Recycle bin cleaning

### In Development
- Browser cache cleaning
- Registry cleaning
- Modern GUI interface
- Scheduled cleaning tasks
- Detailed cleaning reports

## 🛠️ Requirements

- Windows 10 or newer
- Visual Studio 2019 or newer
- CMake 3.10 or newer
- C++17 or newer

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