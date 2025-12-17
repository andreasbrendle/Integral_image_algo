// integral.cpp
// Single- and multi-core integral image (summed-area table) computation
// Build: g++ -O3 -std=c++17 -pthread -march=native -o integral src/integral.cpp
// Optional: compile with -fopenmp to enable the OpenMP variant

#include "integral.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>
#include <random>
#include <thread>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <string>
#include <sstream>

using std::size_t;
using std::vector;
using std::uint32_t;
using std::uint64_t;
using std::cerr;
using std::cout;
using std::endl;

void computeIntegralSingle(const std::vector<u32>& img, std::size_t w, std::size_t h, std::vector<u64>& integral) noexcept{
    if(w==0 || h==0) { integral.clear(); return; }
    integral.assign(w*h, 0);
    for(size_t y=0;y<h;++y){
        u64 row_sum = 0;
        size_t base = y*w;
        for(size_t x=0;x<w;++x){
            row_sum += img[base + x];
            u64 above = (y>0) ? integral[(y-1)*w + x] : 0;
            integral[base + x] = row_sum + above;
        }
    }
}

void computeIntegralMulti(const std::vector<u32>& img, std::size_t w, std::size_t h, std::vector<u64>& integral, int num_threads) noexcept{
    if(w==0 || h==0) { integral.clear(); return; }
    if(num_threads < 1) num_threads = 1;
    integral.assign(w*h, 0);
    vector<u64> rowCum(w*h);

    // Phase 1: per-row prefix sums
    auto worker_rows = [&](int tid){
        size_t rows_per = (h + num_threads - 1) / num_threads;
        size_t y0 = tid * rows_per;
        size_t y1 = std::min(h, y0 + rows_per);
        for(size_t y=y0;y<y1;++y){
            u64 s = 0;
            size_t base = y*w;
            for(size_t x=0;x<w;++x){
                s += img[base + x];
                rowCum[base + x] = s;
            }
        }
    };

    vector<std::thread> threads;
    for(int t=0;t<num_threads;++t) threads.emplace_back(worker_rows, t);
    for(auto &th: threads) th.join();

    // Phase 2: per-column prefix sums over rowCum -> integral
    auto worker_cols = [&](int tid){
        size_t cols_per = (w + num_threads - 1) / num_threads;
        size_t x0 = tid * cols_per;
        size_t x1 = std::min(w, x0 + cols_per);
        for(size_t x=x0;x<x1;++x){
            u64 s = 0;
            for(size_t y=0;y<h;++y){
                s += rowCum[y*w + x];
                integral[y*w + x] = s;
            }
        }
    };
    threads.clear();
    for(int t=0;t<num_threads;++t) threads.emplace_back(worker_cols, t);
    for(auto &th: threads) th.join();
}

#ifdef _OPENMP
#include <omp.h>
void computeIntegralOpenMP(const std::vector<u32>& img, std::size_t w, std::size_t h, std::vector<u64>& integral, int num_threads) noexcept{
    if(w==0 || h==0) { integral.clear(); return; }
    if(num_threads < 1) num_threads = 1;
    integral.assign(w*h, 0);
    vector<u64> rowCum(w*h);

    omp_set_num_threads(num_threads);
#pragma omp parallel for schedule(static)
    for(std::ptrdiff_t y=0;y<static_cast<std::ptrdiff_t>(h);++y){
        u64 s = 0;
        size_t base = static_cast<size_t>(y)*w;
        for(size_t x=0;x<w;++x){
            s += img[base + x];
            rowCum[base + x] = s;
        }
    }

#pragma omp parallel for schedule(static)
    for(std::ptrdiff_t x=0;x<static_cast<std::ptrdiff_t>(w);++x){
        u64 s = 0;
        for(size_t y=0;y<h;++y){
            s += rowCum[y*w + static_cast<size_t>(x)];
            integral[y*w + static_cast<size_t>(x)] = s;
        }
    }
}
#endif

void computeIntegralNaive(const std::vector<u32>& img, std::size_t w, std::size_t h, std::vector<u64>& integral) noexcept{
    if(w==0 || h==0) { integral.clear(); return; }
    integral.assign(w*h, 0);
    for(size_t y=0;y<h;++y){
        for(size_t x=0;x<w;++x){
            u64 s = 0;
            for(size_t i=0;i<=x;++i) for(size_t j=0;j<=y;++j) s += img[j*w + i];
            integral[y*w + x] = s;
        }
    }
}

// Helper: generate random image with deterministic seed
static void randImage(vector<u32>& img, std::size_t w, std::size_t h, uint32_t seed=1337u){
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> d(0,255);
    img.resize(w*h);
    for(auto &v: img) v = (u32)d(rng);
}

// Verify equality
static bool equalIntegral(const std::vector<u64>& a, const std::vector<u64>& b) noexcept{
    if(a.size()!=b.size()) return false;
    for(size_t i=0;i<a.size();++i) if(a[i]!=b[i]) return false;
    return true;
}

// small utility: mean and stddev
static void stats(const std::vector<double>& v, double &mean, double &stddev){
    mean = 0; stddev = 0;
    if(v.empty()) return;
    for(double x: v) mean += x;
    mean /= v.size();
    for(double x: v) stddev += (x-mean)*(x-mean);
    stddev = std::sqrt(stddev / v.size());
}

#ifndef UNIT_TESTS
int main(int argc, char**argv){
    // Default parameters
    size_t w = 2000, h = 1000;
    int threads = static_cast<int>(std::thread::hardware_concurrency());
    int runs = 5;
    uint32_t seed = 1337u;
    std::string method = "both"; // single|multi|both|openmp

    // Simple CLI parsing
    for(int i=1;i<argc;++i){
        std::string s(argv[i]);
        if(s=="--width" && i+1<argc) w = static_cast<size_t>(std::stoul(argv[++i]));
        else if(s=="--height" && i+1<argc) h = static_cast<size_t>(std::stoul(argv[++i]));
        else if(s=="--threads" && i+1<argc) threads = std::stoi(argv[++i]);
        else if(s=="--runs" && i+1<argc) runs = std::stoi(argv[++i]);
        else if(s=="--seed" && i+1<argc) seed = static_cast<uint32_t>(std::stoul(argv[++i]));
        else if(s=="--method" && i+1<argc) method = argv[++i];
        else if(s=="--help"){ cerr<<"Usage: integral [--width W] [--height H] [--threads N] [--runs R] [--seed S] [--method single|multi|both|openmp]\n"; return 0; }
    }

    if(w==0 || h==0) throw std::invalid_argument("width and height must be > 0");
    if(runs <= 0) runs = 1;
    if(threads<=0) threads = 1;

    cerr << "Image: "<< w <<" x "<< h <<"  threads="<<threads<<"  runs="<<runs<<"  seed="<<seed<<"\n";

    vector<u32> img;
    randImage(img, w, h, seed);

    vector<u64> I_ref, I_single, I_multi;

    // Warm-up / correctness check
    computeIntegralSingle(img,w,h,I_single);
    computeIntegralMulti(img,w,h,I_multi, threads);

    if(!equalIntegral(I_single, I_multi)){
        cerr << "ERROR: single and multi implementations differ!\n";
        return 2;
    }

#ifdef _OPENMP
    if(method=="openmp"){
        computeIntegralOpenMP(img,w,h,I_multi, threads);
        if(!equalIntegral(I_single, I_multi)){
            cerr << "ERROR: single and openmp implementations differ!\n";
            return 2;
        }
    }
#endif

    // Benchmarks
    auto bench = [&](const std::string &name, const std::function<void()> &f){
        std::vector<double> times;
        for(int r=0;r<runs;++r){
            auto t0 = std::chrono::high_resolution_clock::now();
            f();
            auto t1 = std::chrono::high_resolution_clock::now();
            times.push_back(std::chrono::duration_cast<std::chrono::duration<double>>(t1-t0).count());
        }
        double mean, stddev; stats(times, mean, stddev);
        cerr << std::fixed << std::setprecision(6);
        cerr << name << ": mean="<< mean << " s  stddev="<< stddev << " s\n";
        return mean;
    };

    double t_single=0, t_multi=0;
    if(method=="both" || method=="single"){
        t_single = bench("Single", [&]{ computeIntegralSingle(img,w,h,I_single); });
    }
    if(method=="both" || method=="multi"){
        t_multi = bench("Multi", [&]{ computeIntegralMulti(img,w,h,I_multi, threads); });
    }
#ifdef _OPENMP
    if(method=="openmp"){
        bench("OpenMP", [&]{ computeIntegralOpenMP(img,w,h,I_multi, threads); });
    }
#endif

    if(t_multi>0 && t_single>0){
        cerr << "Speedup (single / multi) = "<< (t_single / t_multi) <<"\n";
    }

    return 0;
}
#endif // UNIT_TESTS

