cmake_minimum_required(VERSION 3.15)

project(CookieMonster VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Add source files
set(SOURCES
    src/source/Cleaner.cpp
)

# Add header files
set(HEADERS
    src/include/Cleaner.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Set include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include
)

# Set compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    shell32
    shlwapi
)

# Install rules
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install headers
install(FILES ${HEADERS}
    DESTINATION include/${PROJECT_NAME}
)
