@echo off
echo Building CookieMonster...

:: Create build directory if it doesn't exist
if not exist build mkdir build
cd build

:: Configure and build the project
cmake ..
cmake --build . --config Release

:: Return to project root
cd ..

:: Create installer
echo Creating installer...
"C:\Program Files (x86)\NSIS\makensis.exe" installer.nsi

echo Done!
pause 