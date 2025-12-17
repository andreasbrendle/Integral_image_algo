# Integral_image_algo

# Integral Image Algorithm – Single vs. Multi-Threaded Experiments

[![Language](https://img.shields.io/badge/language-C++17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Build](https://img.shields.io/badge/build-Makefile-green.svg)]()
[![Tests](https://img.shields.io/badge/tests-passing-brightgreen)]()

This repository explores efficient implementations of the **integral image** (also known as summed-area table) in C++17.

An integral image allows computing the sum of pixel values over any axis-aligned rectangular region in **O(1)** time after an **O(N)** preprocessing step. It is widely used in computer vision for tasks like:
- Feature extraction (e.g., Viola-Jones face detection)
- Image filtering
- Box blurring
- Area sums in real-time applications

The goal here is to compare:
- A **single-threaded** implementation
- A **multi-threaded** version (using `std::thread`)

## Build and Run Tests

```bash
make                # Build the production code
make test           # Build and run unit tests (or ./run_tests.sh)
make clean          # Clean build artifacts

### Benchmark Example

Tested on a 2000 × 1000 image, 5 runs, seed = 1337, 2 threads

| Implementation   | Mean time (s) | Stddev (s) | Speedup       |
|------------------|---------------|------------|---------------|
| Single-threaded  | 0.003592      | 0.000227   | -             |
| Multi-threaded   | 0.014214      | 0.001655   | 0.25× (slower)|

**Note**: Multi-threaded is slower here because the image is small and thread overhead dominates.  