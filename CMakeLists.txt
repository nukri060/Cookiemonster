cmake_minimum_required(VERSION 3.10)
project(CookieMonster)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add executable
add_executable(cookiemonster main.cpp)

# Link required Windows libraries
target_link_libraries(cookiemonster PRIVATE
    shell32
    user32
) 