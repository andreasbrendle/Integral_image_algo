#!/usr/bin/env bash
set -euo pipefail

echo "Building and running tests..."
make tests
echo "Running test binary:"
./tests/run_tests
echo "Done."
