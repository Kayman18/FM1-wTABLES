#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native validation/test29
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
cc -O2 -ffp-contract=off -fsanitize=address,undefined,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o validation/test29/test-hold tests/test_hold.c $sources -lm
ASAN_OPTIONS=detect_leaks=0 validation/test29/test-hold
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libtest29engine.so fm1/app/test_engine.c $sources
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest28ref.so tests/baseline28/test_engine.c $sources
python3 tests/compare_hold_bypass.py
