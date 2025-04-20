cmake_minimum_required(VERSION 3.15)

add_executable(cookiemonster
    source/main.cpp
    source/Cleaner.cpp
)

target_include_directories(cookiemonster PRIVATE include)
