CXX := g++
CXXFLAGS := -O3 -std=c++17 -pthread -Wall -Wextra -march=native
ifdef OPENMP
CXXFLAGS += -fopenmp
endif

SRC := src/integral.cpp
TESTSRC := tests/test_integral.cpp

.PHONY: all clean tests

all: integral tests

integral: $(SRC) src/integral.hpp
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)

tests: $(TESTSRC) $(SRC) src/integral.hpp
	$(CXX) $(CXXFLAGS) -DUNIT_TESTS -o tests/run_tests $(TESTSRC) $(SRC)

clean:
	rm -f integral tests/run_tests
