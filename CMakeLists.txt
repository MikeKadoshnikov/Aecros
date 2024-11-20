cmake_minimum_required(VERSION 3.28)
project(Aecros)

# Set the C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the source files for your project
add_executable(Aecros
        main.cpp
        window.cpp
        tinyfiledialogs.c
        window.hpp  # Include this if you have the source file in your project
)

# Find required packages
find_package(SFML 2.5 COMPONENTS graphics window system audio REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

# Link SFML libraries
target_link_libraries(Aecros
        sfml-graphics
        sfml-window
        sfml-system
        sfml-audio
        ${GTK3_LIBRARIES}  # Link GTK libraries
)

# Include directories for GTK
target_include_directories(Aecros PRIVATE ${GTK3_INCLUDE_DIRS})

# Optionally, you can set any compiler flags if needed
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(Aecros PRIVATE -Wall)
endif()

