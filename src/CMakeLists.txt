add_executable(cookiemonster) 
 
target_sources(cookiemonster PRIVATE) 
    source/main.cpp 
    source/Cleaner.cpp 
 
target_include_directories(cookiemonster PRIVATE) 
    include 
