#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p validation/test25 native
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
cc -O2 -ffp-contract=off -fsanitize=address,undefined,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o validation/test25/test-chord tests/test_chord.c fm1/app/test_engine.c $sources -lm
ASAN_OPTIONS=detect_leaks=0 validation/test25/test-chord
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libtest25engine.so fm1/app/test_engine.c $sources
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest24ref.so tests/baseline24/test_engine.c $sources
python3 tests/compare_chord_bypass.py
cc -O2 -ffp-contract=off -fsanitize=address,undefined -I fm1/base/include -o validation/test25/test-trigger tests/test_trigger.c fm1/app/test_engine.c $sources -lm
ASAN_OPTIONS=detect_leaks=0 validation/test25/test-trigger
