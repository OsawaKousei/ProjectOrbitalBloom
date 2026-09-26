#!/usr/bin/env bash
set -euo pipefail
core_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
test_dir="$(mktemp -d /tmp/orbital-core-tests.XXXXXX)"
trap 'rm -rf -- "$test_dir"' EXIT
"${CXX:-g++}" -std=c++17 -Wall -Wextra -Wpedantic -Werror -O2 \
    -I"$core_dir/include" "$core_dir/src/Simulation.cpp" \
    "$core_dir/tests/SimulationTests.cpp" -o "$test_dir/tests"
"$test_dir/tests"
