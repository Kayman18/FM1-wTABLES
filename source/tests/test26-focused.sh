#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native validation/test26
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
cc -O2 -ffp-contract=off -fsanitize=address,undefined,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o validation/test26/test-break tests/test_break.c $sources -lm
ASAN_OPTIONS=detect_leaks=0 validation/test26/test-break
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libtest26engine.so fm1/app/test_engine.c $sources
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest25ref.so tests/baseline25/test_engine.c $sources
python3 tests/compare_break_bypass.py
