#!/usr/bin/env bash
# Compiles and runs the host-native unit tests under tests/ with a plain
# gcc (NOT the N64 mips toolchain) — see tests/README.md for why this works
# and what it does/doesn't cover. Intended to run inside the same
# ghcr.io/dragonminded/libdragon Docker image already used to build the ROM
# (it happens to ship a normal host gcc too), but any host gcc/clang works.
set -e
cd "$(dirname "$0")/.."

CC=${CC:-gcc}
FAKES=tests/fakes
FAIL=0

run_suite() {
    local name="$1"; shift
    echo "=== $name ==="
    "$CC" -std=c11 -Wall -I "$FAKES" "$@" -lm -o "/tmp/td64_test_$name"
    if ! "/tmp/td64_test_$name"; then
        FAIL=1
    fi
    echo
}

run_suite score tests/test_score.c src/systems/score.c
run_suite pathfinding tests/test_pathfinding.c src/world/pathfinding.c
run_suite camera tests/test_camera.c src/systems/effects.c
run_suite controls tests/test_controls.c src/systems/controls.c tests/fakes/save_stub.c

if [ "$FAIL" -ne 0 ]; then
    echo "SOME TEST SUITES FAILED"
    exit 1
fi
echo "ALL TEST SUITES PASSED"
