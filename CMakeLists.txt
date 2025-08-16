cmake_minimum_required(VERSION 4.0)

project(TerminalSoftwareRasterizer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "-O2 -Wall -Wextra")

include_directories(include)

add_executable(SoRaCLI
    src/main.cpp
)