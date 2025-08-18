cmake_minimum_required(VERSION 4.0)

project(TerminalSoftwareRasterizer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "-O2 -Wall -Wextra")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)

add_executable(SoRaCLI
    src/main.cpp
    src/img.cpp
)