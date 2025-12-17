// integral.hpp
// Declarations for integral image computations (summed-area table).
// See src/integral.cpp for implementations.

#ifndef INTEGRAL_HPP
#define INTEGRAL_HPP

#include <cstdint>
#include <cstddef>
#include <vector>

using u32 = std::uint32_t;
using u64 = std::uint64_t;

/**
 * Compute the integral image (summed-area table) for a 2D image (single-core).
 *
 * @param img Input image stored row-major (size == w*h).
 * @param w Width of the image (pixels).
 * @param h Height of the image (pixels).
 * @param integral Output buffer: will be resized to w*h and filled with results.
 */
void computeIntegralSingle(const std::vector<u32>& img, std::size_t w, std::size_t h, std::vector<u64>& integral) noexcept;

/**
 * Compute the integral image using multiple threads.
 * Strategy: per-row prefix sums in parallel, then per-column prefix sums in parallel.
 *
 * @param img Input image stored row-major (size == w*h).
 * @param w Width of the image (pixels).
 * @param h Height of the image (pixels).
 * @param integral Output buffer: will be resized to w*h and filled with results.
 * @param num_threads Number of threads to use (>=1).
 */
void computeIntegralMulti(const std::vector<u32>& img, std::size_t w, std::size_t h, std::vector<u64>& integral, int num_threads) noexcept;

#ifdef _OPENMP
void computeIntegralOpenMP(const std::vector<u32>& img, std::size_t w, std::size_t h, std::vector<u64>& integral, int num_threads) noexcept;
#endif

/**
 * Naive reference implementation: O(w*h*avg_area) used for small tests; not intended for benchmarks on large images.
 */
void computeIntegralNaive(const std::vector<u32>& img, std::size_t w, std::size_t h, std::vector<u64>& integral) noexcept;

#endif // INTEGRAL_HPP
