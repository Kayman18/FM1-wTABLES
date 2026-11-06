#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p native validation/test27
sources=$(find fm1/app -maxdepth 1 -name 'pw_*.c' -print)
cc -O2 -ffp-contract=off -fsanitize=address,undefined,float-cast-overflow -fno-sanitize-recover=all -I fm1/base/include -o validation/test27/test-sustain-mode tests/test_sustain_mode.c $sources -lm
ASAN_OPTIONS=detect_leaks=0 validation/test27/test-sustain-mode
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -o native/libtest27engine.so fm1/app/test_engine.c $sources
cc -O2 -ffp-contract=off -shared -fPIC -I fm1/base/include -I fm1/app -o native/libtest26ref.so tests/baseline26/test_engine.c $sources
python3 tests/compare_sustain_bypass.py
