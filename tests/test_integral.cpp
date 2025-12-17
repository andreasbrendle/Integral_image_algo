#include "../src/integral.hpp"
#include <iostream>
#include <vector>
#include <random>
#include <cassert>

using std::cout; using std::endl;

static void test_small_known(){
    // simple 3x3
    std::vector<u32> img = {1,2,3,4,5,6,7,8,9};
    std::vector<u64> A,B,C;
    computeIntegralSingle(img,3,3,A);
    computeIntegralMulti(img,3,3,B,2);
    computeIntegralNaive(img,3,3,C);
    assert(A==B);
    assert(A==C);
}

static void test_random_compare(unsigned w, unsigned h, unsigned seed){
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> d(0,255);
    std::vector<u32> img(w*h);
    for(auto &v: img) v = (u32)d(rng);
    std::vector<u64> A,B;
    computeIntegralSingle(img,w,h,A);
    computeIntegralMulti(img,w,h,B,4);
    assert(A==B);
}

static void test_rect_sum_property(){
    unsigned w=10,h=10;
    std::mt19937 rng(123);
    std::uniform_int_distribution<int> d(0,255);
    std::vector<u32> img(w*h);
    for(auto &v: img) v = (u32)d(rng);
    std::vector<u64> I;
    computeIntegralSingle(img,w,h,I);
    auto rect = [&](int x0,int y0,int x1,int y1){
        // inclusive coordinates
        u64 A = I[y1*w + x1];
        u64 B = (y0>0) ? I[(y0-1)*w + x1] : 0;
        u64 C = (x0>0) ? I[y1*w + (x0-1)] : 0;
        u64 D = (x0>0 && y0>0) ? I[(y0-1)*w + (x0-1)] : 0;
        u64 sum = A - B - C + D;
        u64 sref = 0;
        for(int y=y0;y<=y1;++y) for(int x=x0;x<=x1;++x) sref += img[y*w + x];
        return std::pair<u64,u64>(sum,sref);
    };
    for(int i=0;i<100;++i){
        int x0 = rng()%w, x1 = rng()%w, y0=rng()%h, y1=rng()%h;
        if(x0>x1) std::swap(x0,x1);
        if(y0>y1) std::swap(y0,y1);
        auto p = rect(x0,y0,x1,y1);
        assert(p.first==p.second);
    }
}

int main(){
    cout << "Running tests...\n";
    test_small_known();
    for(unsigned s=0;s<5;++s) test_random_compare(32 + s*8, 16 + s*7, 1000+s);
    test_rect_sum_property();
    cout << "All tests passed."<<endl;
    return 0;
}
