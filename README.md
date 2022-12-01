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
- `python3-config` is not found in my laptop, so I have to explicitly add path of `python310.lib` to CMake `LINKER_LIBS`.

## Hints

Part 8: CUDA Backend - Reductions
- `thrust::reduce` may be helpful to implement reduce kernel on `__device__` side.

Part 9: CUDA Backend - Matrix multiplication
- Phase 1: Naive version, mapping each `out[i][j]` to one thread.
- Phase 2: Shared memory version, performing co-fetch due to locality.
- Phase 3: Tiled version, making each thread heavier workload by calculating `out[i:i+V][j:j+V]` in one go. 
