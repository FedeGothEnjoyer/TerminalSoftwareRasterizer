# SoRaCLI

A simple software rasterizer CLI project.

## Build & Run

From the project root, run:

    mkdir build && cd build && cmake .. && make && ./SoRaCLI

## Requirements

- CMake (3.10+ recommended)
- A C++17-compatible compiler (e.g., g++ or clang++)

## Notes

- Build artifacts go into the `build/` directory.
- Use `make clean` inside `build/` to force a rebuild.
- On Windows, you can use a different generator, e.g.:
  
      cmake .. -G "MinGW Makefiles"
