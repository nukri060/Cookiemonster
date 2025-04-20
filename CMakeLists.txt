cmake_minimum_required(VERSION 3.15)

project(CookieMonster VERSION 1.1.0 LANGUAGES CXX) 
 
set(CMAKE_CXX_STANDARD 17) 
set(CMAKE_CXX_STANDARD_REQUIRED ON) 
 
add_subdirectory(src) 
add_subdirectory(tests) 
