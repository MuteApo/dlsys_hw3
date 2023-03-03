# Homework 3

Public repository and stub/testing code for Homework 3 of 10-714.

## Dev Environment
- Windows 11
- Miniconda3 (with Python 3.10.6)
- Visual Studio 2022 (with Windows 10 SDK)
- NVCC with CUDA 11.7
- CMake 3.19

## Setup Configuration

- `VERSION` specified in `cmake_minimum_required()` will definitely affect how CMake performs (in order to be compatible to older versions). `VERSION=3.4/3.12` cannot properly detect my conda env (with Python 3.10.6) and falls back to the global python (version 3.9.12) executable.
- `python3-config` cannot be found on my laptop, so I have to explicitly add path of `python310.lib` to CMake `LINKER_LIBS`.

## Hints

Part 8: CUDA Backend - Reductions
- `thrust` library (CUDA STL) may be helpful to implement reduce kernel on `__device__` side.

Part 9: CUDA Backend - Matrix multiplication
- Phase 1: Naive version, mapping each `out[i][j]` to one thread.
- Phase 2: Tiled version, each thread calculates `out[i:i+V][j:j+V]` (in total $V\times V$ items). 
- Phase 3: Shared memory version, $L\times L$ threads in one block perform co-fetch due to space locality.
