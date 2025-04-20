@echo off
echo Creating project structure...

:: Create main directories
mkdir src
mkdir docs
mkdir installer
mkdir tests

:: Create src directory structure
mkdir src\include
mkdir src\source

:: Create docs directory structure
mkdir docs\images
mkdir docs\api

:: Create installer directory structure
mkdir installer\resources

:: Create tests directory structure
mkdir tests\unit
mkdir tests\integration

:: Move existing files to new locations
move Cleaner.h src\include\
move Cleaner.cpp src\source\
move main.cpp src\source\
move README.md docs\
move CHANGELOG.md docs\
move RELEASE_NOTES.md docs\
move LICENSE docs\
move installer.nsi installer\
move build_installer.bat installer\

:: Create new CMakeLists.txt files
echo Creating CMakeLists.txt files...

:: Main CMakeLists.txt
echo project(CookieMonster VERSION 1.1.0 LANGUAGES CXX) > CMakeLists.txt
echo. >> CMakeLists.txt
echo set(CMAKE_CXX_STANDARD 17) >> CMakeLists.txt
echo set(CMAKE_CXX_STANDARD_REQUIRED ON) >> CMakeLists.txt
echo. >> CMakeLists.txt
echo add_subdirectory(src) >> CMakeLists.txt
echo add_subdirectory(tests) >> CMakeLists.txt

:: Src CMakeLists.txt
echo add_executable(cookiemonster) > src\CMakeLists.txt
echo. >> src\CMakeLists.txt
echo target_sources(cookiemonster PRIVATE) >> src\CMakeLists.txt
echo     source/main.cpp >> src\CMakeLists.txt
echo     source/Cleaner.cpp >> src\CMakeLists.txt
echo. >> src\CMakeLists.txt
echo target_include_directories(cookiemonster PRIVATE) >> src\CMakeLists.txt
echo     include >> src\CMakeLists.txt

:: Tests CMakeLists.txt
echo enable_testing() > tests\CMakeLists.txt
echo. >> tests\CMakeLists.txt
echo add_subdirectory(unit) >> tests\CMakeLists.txt
echo add_subdirectory(integration) >> tests\CMakeLists.txt

:: Update .gitignore
echo # Build directories > .gitignore
echo build/ >> .gitignore
echo out/ >> .gitignore
echo bin/ >> .gitignore
echo obj/ >> .gitignore
echo. >> .gitignore
echo # IDE files >> .gitignore
echo .vs/ >> .gitignore
echo .vscode/ >> .gitignore
echo *.sln >> .gitignore
echo *.vcxproj >> .gitignore
echo *.vcxproj.filters >> .gitignore
echo *.vcxproj.user >> .gitignore
echo. >> .gitignore
echo # Compiled files >> .gitignore
echo *.exe >> .gitignore
echo *.dll >> .gitignore
echo *.lib >> .gitignore
echo *.obj >> .gitignore
echo *.pdb >> .gitignore
echo. >> .gitignore
echo # CMake generated files >> .gitignore
echo CMakeCache.txt >> .gitignore
echo CMakeFiles/ >> .gitignore
echo *.cmake >> .gitignore
echo Makefile >> .gitignore
echo. >> .gitignore
echo # Logs and databases >> .gitignore
echo *.log >> .gitignore
echo *.sqlite >> .gitignore

echo Project structure created successfully!
pause 